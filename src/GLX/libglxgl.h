#if !defined(__LIB_GLX_GL_H)
#define __LIB_GLX_GL_H

#include <GL/glx.h>
#include "glvnd_pthread.h"

/*
 * Glue header defining the ABI between libGLX and the libGL wrapper library.
 */

/**
 * Called from libGL.so to load a GLX function.
 *
 * \p ptr should point to the variable that will hold the function pointer. If
 * the value is already non-NULL, then __glXGLLoadGLXFunction will assume that
 * the function has already been loaded and will just return \c (*ptr).
 *
 * To avoid problems with multiple threads trying to load the same function at
 * the same time, __glXGLLoadGLXFunction will lock \p mutex before it tries to
 * read or write \p ptr.
 *
 * Also see src/generate/gen_libgl_glxstubs.py for where this is used.
 *
 * \param name The name of the function to load.
 * \param[out] ptr A pointer to store the function in.
 * \param ptr A mutex to lock before accessing \p ptr, or NULL.
 * \return A pointer to the requested function.
 */
extern __GLXextFuncPtr __glXGLLoadGLXFunction(const char *name, __GLXextFuncPtr *ptr, glvnd_mutex_t *mutex);

#endif // !defined(__LIB_GLX_GL_H)
