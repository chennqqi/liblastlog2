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

#include "lastlog2.h"

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#define LASTLOG_PATH "/tmp/var/log/lastlog2/"

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


/* Internal functions */
static int putlstlogent_impl (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex);
static int getlstlogent_impl (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex);

static int try_create_lastlog_dir (const char *const ll_path);

static inline uid_t get_uid_dir (uid_t uid)
{
    return (uid - (uid % 1000));
}

static int try_create_lastlog_dir (const char *const ll_path)
{
    int checked = 0;
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
                errno = saved_errno;
                return -1;
        }

        if (mkdir (ll_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
            /* What if another process created directory? */
            saved_errno = errno;
            if (saved_errno == EEXIST) {
                checked = 1;
                /* Well. Someone could remove directory while another round of checking. */
                goto repeat;
            }
            errno = saved_errno;
            return -1;
        }
        checked = 1;
        goto repeat;
    }

    if (dir_fd != -1) {
        return dir_fd;
    }

    errno = saved_errno;
    return -1;
}

static int check_extension(unsigned int extension_id)
{
    switch (extension_id) {
        case EXTENSION_MAGIC:
            return 1;
        default:
            return 0;
    }

    abort();
}

inline int getlstlogent (const uid_t uid, struct lastlog *const ll)
{
    return getlstlogent_impl (uid, ll, NULL);
}

inline int getlstlogentx (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex)
{
    return getlstlogent_impl (uid, ll, ll_ex);
}

static int getlstlogent_impl (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex)
{
    assert (ll != NULL);


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
        return -1;
    }

    struct stat st;
    if (fstat (ll_fd, &st) == -1) {
        saved_errno = errno;
        close (ll_fd);
        errno = saved_errno;
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        close (ll_fd);
        return -2;
    }

    if (st.st_size < (off_t)sizeof (*ll)) {
        saved_errno = errno;

        UNLOCK_LASTLOG;

        close (ll_fd);
        errno = saved_errno;
        return -1;
    }
    
    LOCK_LASTLOG
    {
        /* FAIL */
        close (ll_fd);
        errno = ENOLCK;
        return -1;
    }

    /* Plain old format. */
    if (st.st_size == sizeof(*ll)) {
        memset (ll, 0, sizeof(*ll));
        const ssize_t n = read (ll_fd, ll, sizeof(*ll));
        if ((n == -1) || (n != sizeof(*ll))) {
            UNLOCK_LASTLOG;
            close (ll_fd);
            return -1;
        }

        UNLOCK_LASTLOG;
        close (ll_fd);
        return 1;
    }

    /* Don't care about extensions. Even if size if bigger than expected... */
    if (ll_ex == NULL) {
        UNLOCK_LASTLOG;
        close (ll_fd);
        return -1;
    }

    /* Format with extensions. */
    if (st.st_size < (off_t)(sizeof (*ll) + sizeof (*ll_ex))) {
        UNLOCK_LASTLOG;
        close (ll_fd);
        return -1;
    }

    struct iovec iov[2] = {
        { .iov_base = (void *) ll, .iov_len = sizeof (*ll) },
        { .iov_base = (void *) ll_ex, .iov_len = sizeof (*ll_ex) }
    };

    memset (ll, 0, sizeof (*ll));
    memset (ll_ex, 0, sizeof (*ll_ex));
    const ssize_t n = readv (ll_fd, iov, (sizeof (iov) / sizeof (iov[0])));
    saved_errno = errno;
    close (ll_fd);
    UNLOCK_LASTLOG;

    if (n == -1) {
        errno = saved_errno;
        return -1;
    }

    /* Allow more extension in future. */
    if (n < (ssize_t)(sizeof (*ll) + sizeof (*ll_ex))) {
        return -2;
    }

    if (check_extension (ll_ex->extension_id))
    {
        return 1;
    }

    /* Be like negative at all cost. */
    return -2;
}

inline int putlstlogent (const uid_t uid, const struct lastlog *const ll)
{
    return putlstlogent_impl (uid, ll, NULL);
}

inline int putlstlogentx (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex)
{
    return putlstlogent_impl (uid, ll, ll_ex);
}

static int putlstlogent_impl (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex)
{
    assert (ll != NULL);


    const uid_t uid_dir = get_uid_dir (uid);

    char ll_path[sizeof_strs2 (LASTLOG_PATH, STR (UID_MAX))] = {0};
    sprintf (ll_path, "%s%u", LASTLOG_PATH, uid_dir);

    const int dir_fd = try_create_lastlog_dir (ll_path);
    if (dir_fd == -1) {
        return -1;
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
        errno = saved_errno;
        return -1;
    }
    /* Close dir handle here. */
    close (dir_fd);

    struct stat st;
    if (fstat(ll_fd, &st) == -1) {
        saved_errno = errno;
        close (ll_fd);
        errno = saved_errno;
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        close (ll_fd);
        return -2;
    }

    LOCK_LASTLOG_WRITE
    {
        saved_errno = errno;
        close (ll_fd);
        errno = saved_errno;
        return -1;
    }

    /* Don't care about extension. */
    if (ll_ex == NULL) {
        const ssize_t n = write (ll_fd, ll, sizeof (*ll));
        saved_errno = errno;

        UNLOCK_LASTLOG;
        close (ll_fd);

        /* Allow reading of extended record as a non-extended record. */
        if (n == -1) {
            errno = saved_errno;
            return -1;
        }

        if (n < (ssize_t)(sizeof(*ll))) {
            return -2;
        }

        return 1;

    } else {

        struct iovec iov[2] = {
            { .iov_base = (void *) ll, .iov_len = sizeof (*ll) },
            { .iov_base = (void *) ll_ex, .iov_len = sizeof (*ll_ex) }
        };
        const ssize_t n = writev (ll_fd, iov, (sizeof (iov) / sizeof (iov[0])));
        saved_errno = errno;

        UNLOCK_LASTLOG;
        close (ll_fd);

        if (n == -1) { 
            errno = saved_errno;
            return -1;
        }

        if (n < (ssize_t)(sizeof(*ll) + sizeof(*ll_ex))) {
            return -2;
        }

        return 1;
    }

    return -2;
}
