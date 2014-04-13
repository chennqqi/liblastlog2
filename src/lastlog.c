#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

#include <lastlog.h>

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

#define UID_MAX ((uid_t) -1)
#define LASTLOG_PATH "/tmp/var/log/lastlog2/"

#define LASTLOG_PATH_LEN (sizeof (LASTLOG_PATH) + sizeof (STR (UID_MAX)) - 1)
#define LASTLOG_FILE_LEN (LASTLOG_PATH_LEN + sizeof(STR (UID_MAX)) - 1)

/* +1 for slash char */
#define LASTLOG_PATH_LEN_PLUS (LASTLOG_PATH + 1)
#define LASTLOG_FILE_LEN_PLUS (LASTLOG_FILE_LEN + 1)

struct lastlog2 {
    struct lastlog old_ll;
    int new_items;
};

static inline uid_t get_uid_dir(uid_t uid)
{
    return (uid - (uid % 1000));
}

int ll_read(const uid_t uid, struct lastlog2 *const ll)
{
    assert (ll != NULL);

    const uid_t uid_dir = get_uid_dir(uid);
    char path[LASTLOG_FILE_LEN_PLUS] = {0};
    sprintf(path, "%s%u/%u", LASTLOG_PATH, uid_dir, uid);

    const int ll_fd = open (path, O_RDWR | O_NOFOLLOW);
    if (ll_fd == -1) {
        perror("no log file");
    }

    struct stat st = {0};
    if (fstat(ll_fd, &st) == -1)
    {
        perror ("stat");
    }

    /* check record size */
    if (st.st_size != sizeof (*ll)) {
        close (ll_fd);
        return -1;
    }

    if (read (ll_fd, ll, sizeof(*ll)) == -1) {
        close (ll_fd);
        perror ("read record");
    }

    close (ll_fd);

    return 1;
}

int ll_add(const uid_t uid, const struct lastlog2 *const ll)
{
    assert (ll != NULL);

    const uid_t uid_dir = get_uid_dir(uid);
    /* We use one \0 char for \ char in sprintf. */
    char ll_path[LASTLOG_PATH_LEN] = {0};
    /* So... we have 3 NUL chars for free.. one we use for /. Second we use as normal and... */
    char true_path[sizeof("/proc/self/fd/") + sizeof(STR (INT_MAX)) + sizeof(STR (UID_MAX)) - 1];
    sprintf (ll_path, "%s%u", LASTLOG_PATH, uid_dir);

    int checked = 0;
    /* We like C right? ; is just empty statement... is better than empty block. */
repeat: ;
    const int dir_fd = open (ll_path, O_DIRECTORY);
    if (!checked && (dir_fd == -1)) {
        if (mkdir (ll_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
            perror ("create dir");
        }
        checked = 1;
        goto repeat;
    }

    sprintf (true_path, "%s%u/%u", "/proc/self/fd/", dir_fd, uid);
    if (dir_fd == -1) {
        perror ("die");
    }

    const int ll_fd = open (true_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (ll_fd == -1) {
        close (dir_fd);
        perror ("ll_file");
    }

    if (write(ll_fd, ll, sizeof(*ll)) == -1) {
        close (ll_fd);
        close (dir_fd);
        perror ("write fail");
    }

    close (ll_fd);
    close (dir_fd);
    return 1;
}

int main(int argc, char **argv)
{
    struct lastlog2 ll = {0};
    ll_add(1034, &ll);
    ll_add(1000, &ll);
    ll_add(999, &ll);
    ll_add(10, &ll);
    ll_add(11034, &ll);
    ll_add(0, &ll);
    ll_add((uid_t)-1, &ll);
    ll_add((uid_t)(((uid_t)-1) / (uid_t)2), &ll);

    uid_t p = 0;
    for (p = 0; p < UID_MAX / 100000; ++p) {
        ll_add(p, &ll);
    }
}
