#include <lastlog.h>

#include "lastlog2.h"


int main (int argc, char **argv)
{
    struct lastlog ll = {0};
    struct ll_extension ll_ex = {0};

    putlstlogentx (1034, &ll, &ll_ex);
    putlstlogentx (1000, &ll, &ll_ex);
    putlstlogentx (999, &ll, &ll_ex);
    putlstlogentx (10, &ll, &ll_ex);
    putlstlogentx (11034, &ll, &ll_ex);
    putlstlogentx (0, &ll, &ll_ex);
    putlstlogentx ((uid_t)-1, &ll, &ll_ex);
    putlstlogentx ((uid_t)(((uid_t)-1) / (uid_t)2), &ll, &ll_ex);

    uid_t p = 0;
    for (p = 0; p < UID_MAX / 10000; ++p) {
        putlstlogentx (p, &ll, &ll_ex);
    }
}
