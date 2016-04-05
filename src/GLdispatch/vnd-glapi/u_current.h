#ifndef _U_CURRENT_H_
#define _U_CURRENT_H_

#include "glapi.h"

/* ugly renames to match glapi.h */

void
u_current_init(void);

void
u_current_destroy(void);

void
u_current_set_multithreaded(void);

/**
 * Set the per-thread dispatch table pointer.
 */
void u_current_set(const struct _glapi_table *tbl);

/**
 * Return pointer to current dispatch table for calling thread.
 */
const struct _glapi_table *u_current_get(void);

#endif /* _U_CURRENT_H_ */

