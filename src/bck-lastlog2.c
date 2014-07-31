#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/uio.h>
#ifndef __APPLE__ && __MACH__
#include <lastlog.h>
#else
#include <utmp.h>
#endif

#include "bck-lastlog2.h"
#include "backend.h"

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#define LASTLOG_PATH "/var/log/lastlog2/"

#define sizeof_strs2(x, y)      ((sizeof((x)) - 1) + sizeof((y)))
#define sizeof_strs3(x, y, z)   ((sizeof((x)) - 1) + (sizeof((y)) - 1) + (sizeof((z))))

#define EXTENSION_MAGIC 1

#define LOCK_LASTLOG \
    struct flock ll_lock = {0}; \
    /* All other attributes are set by initialization. */ \
    ll_lock.l_type = F_RDLCK; \
    ll_lock.l_whence = SEEK_SET; \
    int __ret = -1; \
    do { \
        __ret = fcntl (ll_fd, F_SETLK, &ll_lock); \
    } while ((__ret == -1) && (errno == EINTR)); \
    if (__ret == -1)

#define LOCK_LASTLOG_WRITE \
    struct flock ll_lock = {0}; \
    /* All other attributes are set by initialization. */ \
    ll_lock.l_type = F_WRLCK; \
    ll_lock.l_whence = SEEK_SET; \
    int __ret = -1; \
    do { \
        __ret = fcntl (ll_fd, F_SETLKW, &ll_lock); \
    } while ((__ret == -1) && (errno == EINTR)); \
    if (__ret == -1)

#define UNLOCK_LASTLOG \
    do { \
        struct flock ll_lock = {0}; \
        ll_lock.l_type = F_UNLCK; \
        ll_lock.l_whence = SEEK_SET; \
        fcntl (ll_fd, F_SETLK, &ll_lock); \
    } while (0)

const jump_tbl_t ll_bck_jump_tbl = {
    .init           = NULL, 
    .putent         = &putent,
	.getent         = &getent,
    .fini           = NULL,
	.backend_type   = LL_LASTLOG2,
};

/* Internal functions _NOT_ exported via jump table */
static ssize_t read_all (int fd, void *const buff, ssize_t len);
static ssize_t write_all (int fd, void *const buff, ssize_t len);

/* Internal functions */
static retcode_t try_create_lastlog_dir (const char *const ll_path, int *const fd);

static inline uid_t get_uid_dir (uid_t uid)
{
    return (uid - (uid % 1000));
}

static retcode_t try_create_lastlog_dir (const char *const ll_path, int *const fd)
{
    int checked = 0;
    *fd = -1;
    /* Repeat statement is for 2 purposes here. */
repeat: ;
    /* Here is little race condition */
    const int dir_fd = open (ll_path, O_DIRECTORY | O_NOFOLLOW);
    int saved_errno = errno;

    if ((dir_fd == -1) && (saved_errno == EINTR)) {
        goto repeat;
    }

    if (!checked && (dir_fd == -1)) {
        /* Don't try to create dir when those errnos happened. */
        switch (saved_errno) {
            case EACCES:
            case ENOTDIR:
            case ELOOP:
                return -saved_errno;
        }

        if (mkdir (ll_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
            /* What if another process created directory? */
            saved_errno = errno;
            if (saved_errno == EEXIST) {
                checked = 1;
                /* Well. Someone could remove directory while another round of checking. */
                goto repeat;
            }
            return -saved_errno;
        } else {
            checked = 1;
            goto repeat;
        }
    }

    if (dir_fd != -1) {
        *fd = dir_fd;
        return LASTLOG2_OK;
    }

    return -saved_errno;
}

static int getent (llent_t *const ent)
{
    assert (ent != NULL);

    struct lastlog ll;

    uid_t uid = ent->uid;
    const uid_t uid_dir = get_uid_dir (uid);
    /* ... +1 for slash char */
    char path[sizeof_strs3 (LASTLOG_PATH, STR (UID_MAX), STR (UID_MAX)) + 1] = {0};
    sprintf (path, "%s%u/%u", LASTLOG_PATH, uid_dir, uid);

    int ll_fd = -1;
    do {
       ll_fd = open (path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    } while ((ll_fd == -1) && (errno == EINTR));
    int saved_errno = errno;
    if (ll_fd == -1) {
        return -saved_errno;
    }
    
    LOCK_LASTLOG
    {
        /* FAIL */
        saved_errno = errno;
        close (ll_fd);
        return -saved_errno;
    }

    struct stat st;
    if (fstat (ll_fd, &st) == -1) {
        saved_errno = errno;
        UNLOCK_LASTLOG;
        close (ll_fd);
        return -saved_errno;
    }

    if (!S_ISREG(st.st_mode)) {
        UNLOCK_LASTLOG;
        close (ll_fd);
        return LASTLOG2_ERR;
    }

    if (st.st_size < (off_t)sizeof (ll)) {
        UNLOCK_LASTLOG;
        close (ll_fd);
        return LASTLOG2_ERR;
    }

    /* Plain old format. */
    if (st.st_size < (off_t) sizeof(ll)) {
        return LASTLOG2_ERR;
    }

    memset (&ll, 0, sizeof(ll));
    const ssize_t n = read_all (ll_fd, &ll, sizeof(ll));
    saved_errno = errno;
    UNLOCK_LASTLOG;
    close (ll_fd);

    if (n == -1) {
        return -saved_errno;
    }

    if (n != sizeof(ll)) {
        return LASTLOG2_ERR;
    }

    /* If struct will be bigger, just return zeroes */
    memset (ent, 0, sizeof(ent));

    /* Ok. I didn't use getters/setters here. */
    ent->time = ll.ll_time;
    strncpy (ent->line, ll.ll_line, sizeof(ent->line) - 1);
    strncpy (ent->host, ll.ll_host, sizeof(ent->host) - 1);

    return LASTLOG2_OK;
}

static int putent (const llent_t *const ent)
{
    assert (ent != NULL);

    uid_t uid = ent->uid;
    const uid_t uid_dir = get_uid_dir (uid);

    char ll_path[sizeof_strs2 (LASTLOG_PATH, STR (UID_MAX))] = {0};
    sprintf (ll_path, "%s%u", LASTLOG_PATH, uid_dir);

    int dir_fd = -1;
    if (try_create_lastlog_dir (ll_path, &dir_fd) != LASTLOG2_OK) {
        return LASTLOG2_ERR;
    }

    /* ... + 1 for slash char */
    char ll_file [sizeof_strs3("/dev/fd/", STR (INT_MAX), STR (UID_MAX)) + 1] = {0};
    sprintf (ll_file, "/dev/fd/%u/%u", dir_fd, uid);

try_open_again: ;
    const int ll_fd = open (ll_file, O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    int saved_errno = errno;
    if (ll_fd == -1) {
        if (saved_errno == EINTR) {
            goto try_open_again;
        }
        close (dir_fd);
        return -saved_errno;
    }
    /* Close dir handle here. */
    close (dir_fd);

    LOCK_LASTLOG_WRITE
    {
        saved_errno = errno;
        close (ll_fd);
        return -saved_errno;
    }

    struct stat st;
    if (fstat(ll_fd, &st) == -1) {
        saved_errno = errno;
        UNLOCK_LASTLOG;
        close (ll_fd);
        return -saved_errno;
    }

    if (!S_ISREG(st.st_mode)) {
        UNLOCK_LASTLOG;
        close (ll_fd);
        return LASTLOG2_ERR; 
    }

    struct lastlog ll;
    memset (&ll, 0, sizeof(ll));
    ll.ll_time = ent->time;
    strncpy (ll.ll_line, ent->line, sizeof(ll.ll_line) - 1);
    strncpy (ll.ll_host, ent->host, sizeof(ll.ll_host) - 1);

    const ssize_t n = write_all (ll_fd, &ll, sizeof(ll));
    saved_errno = errno;

    UNLOCK_LASTLOG;
    close (ll_fd);

    /* Allow reading of extended record as a non-extended record. */
    if (n == -1) {
        return -saved_errno;
    }

    /* Maybe format will be changed in future */
    if (n >= (ssize_t)(sizeof(ll))) {
        return LASTLOG2_OK;
    }

    return LASTLOG2_ERR;
}

static ssize_t read_all (int fd, void *const buff, ssize_t len)
{
    ssize_t total = 0;
    int ret;
    while (total < len) {
        do {
            ret = read (fd, buff + total, len - total);
        } while ((ret == -1) && (errno == EINTR));
        /* Something failed bad... */
        if (ret <= 0) { return LASTLOG2_ERR; }
        total += ret;
    }

    return total;
}

static ssize_t write_all (int fd, void *const buff, ssize_t len)
{
    ssize_t total = 0;
    int ret;
    while (total < len) {
        do {
            ret = write (fd, buff + total, len - total);
        } while ((ret == -1) && (errno == EINTR));
        /* Something failed bad... */
        if (ret <= 0) { return LASTLOG2_ERR; }
        total += ret;
    }

    return total;
}
