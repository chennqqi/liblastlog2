#ifndef _H_LASTLOG2_
#define _H_LASTLOG2_
struct ll_extension {
    unsigned int extension_id;
    unsigned long fail_logs;
    char reserved[512];
};

/* TODO:  Add attributes. */
/* Public accessible functions. */
inline int get_lastlog (const uid_t uid, struct lastlog *const ll);
inline int get_lastlog_ex (const uid_t uid, struct lastlog *const ll, struct ll_extension *const ll_ex);

inline int add_lastlog (const uid_t uid, const struct lastlog *const ll);
inline int add_lastlog_ex (const uid_t uid, const struct lastlog *const ll, const struct ll_extension *const ll_ex);

#endif /* _H_LASTLOG2_ */
