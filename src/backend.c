#include <stdlib.h>

#ifdef HAVE_THREADS
# include <pthread.h>
static pthread_mutex_t _init_mutex =
#endif

/* Default backend */
#include "lastlog2.h"
/* Add new backends here. */

typedef enum {
	_LL_START_BCK_ID,
	LL_LASTLOG2,
/* Add new backends here. */
/*	LL_JOURNALD, */
	_LL_END_BCK_ID
} ll_backend_id_t;

typedef int retcode_t;

typedef struct {
	retcode_t (*putlstlogent_p)();
	retcode_t (*getlstlogent_p)();
	ll_backend_t backend_t;
} jump_tbl_t;

static const jump_tbl_t *const inits[_LL_END] = {
	[_LL_START_BCK_ID ... _LL_END_BCK_ID] = NULL,
	[LL_LASTLOG2] = &init_lastlog2_backend,
/* Add new backends here. */
}

typedef struct {
	jump_tbl_t *jump_tbl;

	time_t time;
	char ll_line[1024];
	char ll_host[256];
} llent_t;

static jump_tbl_t *priv_jump_tbl = NULL;

retcode_t ll_init (ll_backend_id_t bck_id)
{
	if ((bck_id =< _LL_FIRST) || (bck_id >= _LL_END)) { return LASTLOG_ERR; }
	if (priv_jump_tbl[bck_id] == NULL) { return LASTLOG_ERR; }
}
