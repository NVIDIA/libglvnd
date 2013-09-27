#ifndef _U_CURRENT_H_
#define _U_CURRENT_H_

#if defined(MAPI_MODE_UTIL) || defined(MAPI_MODE_GLAPI) || \
    defined(MAPI_MODE_BRIDGE)

#include "glapi/glapi.h"

/* ugly renames to match glapi.h */
#define mapi_table _glapi_table

enum {
    U_CURRENT_TABLE = GLAPI_CURRENT_DISPATCH,
    U_CURRENT_USER0 = GLAPI_CURRENT_CONTEXT,
    U_CURRENT_USER1 = GLAPI_CURRENT_USER1,
    U_CURRENT_USER2 = GLAPI_CURRENT_USER2,
    U_CURRENT_USER3 = GLAPI_CURRENT_USER3,
    U_CURRENT_NUM_ENTRIES = GLAPI_NUM_CURRENT_ENTRIES
};

#ifdef GLX_USE_TLS
#define u_current _glapi_tls_Current
#else
#define u_current _glapi_Current
#endif

#define u_current_get_internal _glapi_get_dispatch
#define u_current_get_user_internal _glapi_get_context

#define u_current_tsd _gl_CurrentTSD

#else /* MAPI_MODE_UTIL || MAPI_MODE_GLAPI || MAPI_MODE_BRIDGE */

#include "u_compiler.h"

struct mapi_table;

enum {
    U_CURRENT_TABLE = 0,
    U_CURRENT_USER0,
    U_CURRENT_USER1,
    U_CURRENT_USER2,
    U_CURRENT_USER3,
    U_CURRENT_NUM_ENTRIES
};

#ifdef GLX_USE_TLS

extern __thread void *u_current[U_CURRENT_NUM_ENTRIES]
    __attribute__((tls_model("initial-exec")));

#else /* GLX_USE_TLS */

extern void *u_current[U_CURRENT_NUM_ENTRIES];

#endif /* GLX_USE_TLS */

#endif /* MAPI_MODE_UTIL || MAPI_MODE_GLAPI || MAPI_MODE_BRIDGE */

void
u_current_init(void);

void
u_current_destroy(void);

void
u_current_set(const struct mapi_table *tbl);

void
u_current_set_index(void *p, int index);

void *
u_current_get_index(int index);

struct mapi_table *
u_current_get_internal(void);

void
u_current_set_user(const void *ptr);

void *
u_current_get_user_internal(void);

static INLINE const struct mapi_table *
u_current_get(void)
{
#ifdef GLX_USE_TLS
   return u_current[U_CURRENT_TABLE];
#else
   return (likely(u_current[U_CURRENT_TABLE]) ?
         u_current[U_CURRENT_TABLE] : u_current_get_internal());
#endif
}

static INLINE const void *
u_current_get_user(void)
{
#ifdef GLX_USE_TLS
   return u_current[U_CURRENT_USER0];
#else
   return likely(u_current[U_CURRENT_USER0]) ?
       u_current[U_CURRENT_USER0] : u_current_get_user_internal();
#endif
}

#endif /* _U_CURRENT_H_ */
