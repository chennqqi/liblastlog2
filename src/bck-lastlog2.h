#ifndef _BCK_LASTLOG2_H
#define _BCK_LASTLOG2_H

#include "lastlog2.h"

/* Internal functions exported via jump table */
static int getent (llent_t *const ent) __attribute__ ((warn_unused_result, unused));
static int putent (const llent_t *const ent) __attribute__ ((warn_unused_result, unused));
static void set_line(llent_t *const ent, const char *const line) __attribute__ ((unused));
static const char *get_line(const llent_t *const ent) __attribute__ ((unused));
static void set_host(llent_t *const ent, const char *const host) __attribute__ ((unused));
static const char *get_host(const llent_t *const ent) __attribute__ ((unused));

/* Externaly visible */
const jump_tbl_t ll_bck_jump_tbl;

#endif /* _BCK_LASTLOG2_H */
