#include <stdlib.h>
#include <time.h>

#include "backend.h"
/* Default backend */
#include "bck-lastlog2.h"
/* Add new backends here. */


/* Initialize jump table */
static const jump_tbl_t *const jmp_tables[_LL_END + 1] = {
	[_LL_START ... _LL_END] = NULL,
	[LL_LASTLOG2] = &ll_bck_jump_tbl,
/* Add new backends here. */
};

static const jump_tbl_t *priv_jump_tbl = NULL;

retcode_t ll_init (ll_backend_id_t bck_id)
{
    if (priv_jump_tbl != NULL) { return LASTLOG2_OK; }
	if ((bck_id <= _LL_START) || (bck_id >= _LL_END)) { return LASTLOG2_ERR; }
	if (jmp_tables[bck_id] == NULL) { return LASTLOG2_ERR; }
    if (jmp_tables[bck_id]->init == NULL) { return LASTLOG2_OK; }

    priv_jump_tbl = jmp_tables[bck_id];
    /* Call module init function. */
    return priv_jump_tbl->init();
}

retcode_t ll_putent (const llent_t *const ent)
{
    if (priv_jump_tbl != NULL) { return LASTLOG2_ERR; }
    if (ent == NULL) { return LASTLOG2_ERR; }
    return priv_jump_tbl->putent(ent);
}

retcode_t ll_getent (llent_t *const ent)
{
    if (priv_jump_tbl != NULL) { return LASTLOG2_ERR; }
    if (ent == NULL) { return LASTLOG2_ERR; }
    return priv_jump_tbl->getent(ent);
}
