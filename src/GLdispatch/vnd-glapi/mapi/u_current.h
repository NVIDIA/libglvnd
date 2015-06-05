#ifndef _U_CURRENT_H_
#define _U_CURRENT_H_

#include "glapi/glapi.h"

/* ugly renames to match glapi.h */
#define mapi_table _glapi_table

#ifdef GLX_USE_TLS
#define u_current _glapi_tls_Current
#else
#define u_current _glapi_Current
#endif

#define u_current_get_internal _glapi_get_dispatch

void
u_current_init(void);

void
u_current_destroy(void);

void
u_current_set_multithreaded(void);

void
u_current_set(const struct mapi_table *tbl);

struct mapi_table *
u_current_get_internal(void);

static INLINE const struct mapi_table *
u_current_get(void)
{
#ifdef GLX_USE_TLS
   return u_current[GLAPI_CURRENT_DISPATCH];
#else
   return (likely(u_current[GLAPI_CURRENT_DISPATCH]) ?
         u_current[GLAPI_CURRENT_DISPATCH] : u_current_get_internal());
#endif
}

#endif /* _U_CURRENT_H_ */

