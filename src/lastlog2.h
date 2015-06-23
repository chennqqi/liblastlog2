#ifndef _BACKEND_H
#define _BACKEND_H

#include <time.h>
#include <unistd.h>

typedef enum {
	_LL_START,
	LL_LASTLOG2,
/* Add new backends here. */
/*	LL_JOURNALD, */
	_LL_END
} ll_backend_id_t;

typedef enum {
    LASTLOG2_ERR = 0,
    LASTLOG2_OK = 1
} retcode_t;

/* Public visible macros */
#define LL_BCK_LINE_LEN 1024
#define LL_BCK_HOST_LEN 256

typedef struct {
	time_t time;
	char line[LL_BCK_LINE_LEN];
	char host[LL_BCK_HOST_LEN];
    uid_t uid;
} llent_t;

typedef struct {
    int (*init)(void);
	int (*putent)(const llent_t *const ent);
	int (*getent)(llent_t *const ent);
    void (*set_line)(llent_t *const ent, const char *const line);
    const char *(*get_line)(const llent_t *const ent);
    void (*set_host)(llent_t *const ent, const char *const host);
    const char *(*get_host)(const llent_t *const ent);
    int (*fini)(void);
	ll_backend_id_t backend_type;
} jump_tbl_t;

/* Public functions */
int ll_getent (llent_t *const ent);
int ll_putent (const llent_t *const ent);
int ll_init (ll_backend_id_t bck_id);

#endif /* _BACKEND_H */
