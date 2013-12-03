#ifndef __libgl_h__
#define __libgl_h__

#include "libglxgl.h"

typedef __GLXextFuncPtr (*__GLXGetCachedProcAddressPtr)(const GLubyte *);

extern void __glXWrapperInit(__GLXGetCachedProcAddressPtr pGetProcAddress);

#endif
