#include <lastlog.h>

#include "lastlog2.h"


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
