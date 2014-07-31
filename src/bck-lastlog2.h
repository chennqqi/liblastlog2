#ifndef _BCK_LASTLOG2_H
#define _BCK_LASTLOG2_H

#include "backend.h"

/* Internal functions exported via jump table */
static int getent (llent_t *const ent) __attribute__ ((warn_unused_result));
static int putent (const llent_t *const ent) __attribute__ ((warn_unused_result));

/* Externaly visible */
const jump_tbl_t ll_bck_jump_tbl;

#define UID_MAX ((uid_t) -1)

#endif /* _BCK_LASTLOG2_H */
