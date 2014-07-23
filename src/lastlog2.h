#ifndef _H_LASTLOG2_
#define _H_LASTLOG2_

#include <lastlog.h>

enum {
    LASTLOG2_ERR = -1,
    LASTLOG2_OK = 0
};

struct ll_extension {
    unsigned int extension_id;
    unsigned long fail_logs;
    char reserved[512];
};

#define UID_MAX ((uid_t) -1)

/* Public accessible functions. */
inline int getlstlogent (const uid_t uid, struct lastlog *const ll) __attribute__ ((warn_unused_result));
inline int getlstlogentx (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex) __attribute__ ((warn_unused_result));

inline int putlstlogent (const uid_t uid, const struct lastlog *const ll) __attribute__ ((warn_unused_result));
inline int putlstlogentx (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex) __attribute__ ((warn_unused_result));

#endif /* _H_LASTLOG2_ */
