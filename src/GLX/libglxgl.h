#if !defined(__LIB_GLX_GL_H)
#define __LIB_GLX_GL_H

#include <GL/glx.h>

/*
 * Glue header defining the ABI between libGLX and the libGL wrapper library.
 */

extern __GLXextFuncPtr __glXGetCachedProcAddress(const GLubyte *procName);

#endif // !defined(__LIB_GLX_GL_H)
