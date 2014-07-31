#ifndef _H_LASTLOG2_
#define _H_LASTLOG2_

#include "backend.h"

/* Internal functions exported via jump table */
static int getent (llent_t *const ent) __attribute__ ((warn_unused_result));
static int putent (const llent_t *const ent) __attribute__ ((warn_unused_result));

/* Externaly visible */
const jump_tbl_t ll_bck_jump_tbl = {
    init:               NULL, 
    putent:             putent,
	getent:             getent,
    fini:               NULL,
	backend_type:       LL_LASTLOG2,
};

#define UID_MAX ((uid_t) -1)

#endif /* _H_LASTLOG2_ */
