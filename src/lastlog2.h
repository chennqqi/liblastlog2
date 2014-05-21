#ifndef _H_LASTLOG2_
#define _H_LASTLOG2_

#include <lastlog.h>

struct ll_extension {
    unsigned int extension_id;
    unsigned long fail_logs;
    char reserved[512];
};

#define UID_MAX ((uid_t) -1)

/* TODO:  Add attributes. */
/* Public accessible functions. */
inline int getlstlogent (const uid_t uid, struct lastlog *const ll);
inline int getlstlogentx (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex);

inline int putlstlogent (const uid_t uid, const struct lastlog *const ll);
inline int putlstlogentx (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex);

#endif /* _H_LASTLOG2_ */
