#include <sys/uio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <sys/file.h>

#include <lastlog.h>
#include <stdio.h>


#include "lastlog2.h"


#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#define UID_MAX ((uid_t) -1)
#define LASTLOG_PATH "/tmp/var/log/lastlog2/"

#define LASTLOG_PATH_LEN (sizeof (LASTLOG_PATH) + sizeof (STR (UID_MAX)) - 1)
#define LASTLOG_FILE_LEN (LASTLOG_PATH_LEN + sizeof(STR (UID_MAX)) - 1)

/* +1 for slash char */
#define LASTLOG_PATH_LEN_PLUS (LASTLOG_PATH + 1)
#define LASTLOG_FILE_LEN_PLUS (LASTLOG_FILE_LEN + 1)

#define EXTENSION_MAGIC 1

/* Internal functions */
static int add_lastlog_impl (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex);
static int get_lastlog_impl (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex);

static int lock_lastlog_read (const int ll_fd);
static int unlock_lastlog_read (const int ll_fd);
static int lock_lastlog_write (const int ll_fd);
static int unlock_lastlog_write (const int ll_fd);

static int lock_lastlog_read (const int ll_fd)
{
    flock (ll_fd, LOCK_SH);
}

static int unlock_lastlog_read (const int ll_fd)
{
    flock (ll_fd, LOCK_UN);
}

static int lock_lastlog_write (const int ll_fd)
{
    flock (ll_fd, LOCK_EX);
}
static int unlock_lastlog_write (const int ll_fd)
{
    flock (ll_fd, LOCK_UN);
}

static inline uid_t get_uid_dir (uid_t uid)
{
    return (uid - (uid % 1000));
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


inline int get_lastlog (const uid_t uid, struct lastlog *const ll)
{
    return get_lastlog_impl (uid, ll, NULL);
}

inline int get_lastlog_ex (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex)
{
    return get_lastlog_impl (uid, ll, ll_ex);
}

static int get_lastlog_impl (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex)
{
    assert (ll != NULL);

    const uid_t uid_dir = get_uid_dir (uid);
    char path[LASTLOG_FILE_LEN_PLUS] = {0};
    sprintf (path, "%s%u/%u", LASTLOG_PATH, uid_dir, uid);

    const int ll_fd = open (path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (ll_fd == -1) {
        perror ("no log file");
        return -1;
    }

    lock_lastlog_read (ll_fd);

    struct stat st = {0};
    if ((fstat (ll_fd, &st) == -1) 
        || (st.st_size < (off_t)sizeof (*ll)))
    {
        unlock_lastlog_read (ll_fd);
        close (ll_fd);
        perror ("stat");
        return -1;
    }

    /* Plain old format. */
    if (st.st_size == sizeof(*ll)) {
        memset (ll, 0, sizeof(*ll));
        const ssize_t n = read (ll_fd, ll, sizeof(*ll));
        if ((n == -1) || (n != sizeof(*ll))) {
            unlock_lastlog_read (ll_fd);
            close (ll_fd);
            perror ("read record");
            return -1;
        }

        unlock_lastlog_read (ll_fd);
        close (ll_fd);
        return 1;
    }

    /* Don't care about extensions. But size if bigger than expected... */
    if (ll_ex == NULL) {
        unlock_lastlog_read (ll_fd);
        close (ll_fd);
        return -1;
    }

    /* Format with extensions */
    if (st.st_size < (off_t)(sizeof (*ll) + sizeof (*ll_ex))) {
        unlock_lastlog_read (ll_fd);
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
    /* Allow more extension in future. */
    if ((n == -1) || (n < (ssize_t)(sizeof (*ll) + sizeof (*ll_ex)))) {
        unlock_lastlog_read (ll_fd);
        close (ll_fd);
        perror ("read record");
        return -1;
    }
    unlock_lastlog_read (ll_fd);
    close (ll_fd);

    if (check_extension (ll_ex->extension_id))
    {
        return 1;
    }

    /* Be like negative at all cost. */
    return -1;
}

inline int add_lastlog (const uid_t uid, const struct lastlog *const ll)
{
    return add_lastlog_impl (uid, ll, NULL);
}

inline int add_lastlog_ex (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex)
{
    return add_lastlog_impl (uid, ll, ll_ex);
}

static int add_lastlog_impl (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex)
{
    assert (ll != NULL);

    const uid_t uid_dir = get_uid_dir (uid);
    /* We use one \0 char for \ char in sprintf. */
    char ll_path[LASTLOG_PATH_LEN] = {0};
    /* So... we have 3 NUL free chars.. one is used for backsladh . Second is used as normal
     * terminating char and third is removed. */
    char true_path[sizeof ("/proc/self/fd/") + sizeof (STR (INT_MAX)) + sizeof (STR (UID_MAX)) - 1];
    sprintf (ll_path, "%s%u", LASTLOG_PATH, uid_dir);

    int checked = 0;
    /* We like C right? ; is just empty statement... is better than empty block. */
repeat: ;
    /* Here is little race condition */
    const int dir_fd = open (ll_path, O_DIRECTORY | O_NOFOLLOW);
    if (!checked && (dir_fd == -1)) {
        if (mkdir (ll_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
            /* What if another process created directory? */
            if (errno == EEXIST) {
                checked = 1;
                goto repeat;
            }
            perror ("create dir");
            return -1;
        }
        checked = 1;
        goto repeat;
    }

    if (dir_fd == -1) {
        perror ("die");
        return -1;
    }


    sprintf (true_path, "/proc/self/fd/%u/%u", dir_fd, uid);
    const int ll_fd = open (true_path, O_WRONLY | O_CREAT | O_CLOEXEC | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (ll_fd == -1) {
        close (dir_fd);
        perror ("ll_file");
        return -1;
    }
    close (dir_fd);

    lock_lastlog_write (ll_fd);

    /* Don't care about extension. */
    if (ll_ex == NULL) {
        const ssize_t n = write (ll_fd, ll, sizeof (*ll));
        /* Allow reading of extended record as a non extended record */
        if ((n == -1) || n < (ssize_t)(sizeof(*ll))) {
            unlock_lastlog_write (ll_fd);
            close (ll_fd);
            perror ("write fail");
            return -1;
        }
        unlock_lastlog_write (ll_fd);
        close (ll_fd);
        return 1;
    }

    struct iovec iov[2] = {
        { .iov_base = (void *) ll, .iov_len = sizeof (*ll) },
        { .iov_base = (void *) ll_ex, .iov_len = sizeof (*ll_ex) }
    };
    const ssize_t n = writev (ll_fd, iov, (sizeof (iov) / sizeof (iov[0])));

    unlock_lastlog_write (ll_fd);

    close (ll_fd);
    if ((n == -1) || n < (ssize_t)(sizeof (*ll) + sizeof (*ll_ex)))
    {
        perror ("fail");
        return -1;
    } else {
        return 1;
    }

    return -1;
}

int main (int argc, char **argv)
{
    struct lastlog ll = {0};
    struct ll_extension ll_ex = {0};

    add_lastlog_ex (1034, &ll, &ll_ex);
    add_lastlog_ex (1000, &ll, &ll_ex);
    add_lastlog_ex (999, &ll, &ll_ex);
    add_lastlog_ex (10, &ll, &ll_ex);
    add_lastlog_ex (11034, &ll, &ll_ex);
    add_lastlog_ex (0, &ll, &ll_ex);
    add_lastlog_ex ((uid_t)-1, &ll, &ll_ex);
    add_lastlog_ex ((uid_t)(((uid_t)-1) / (uid_t)2), &ll, &ll_ex);

    uid_t p = 0;
    for (p = 0; p < UID_MAX / 100000; ++p) {
        add_lastlog_ex (p, &ll, &ll_ex);
    }
}
