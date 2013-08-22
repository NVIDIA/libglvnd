/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
 */

#if !defined(__LIB_GLX_MAPPING_H)
#define __LIB_GLX_MAPPING_H

#include "libglxabi.h"

const __GLXdispatchTable* __glXGetDispatch(Display *dpy, const int screen);
const __GLXdispatchTable *__glXGetDispatchFromTLS(void);

void __glXAddScreenContextMapping(GLXContext context, int screen);
void __glXRemoveScreenContextMapping(GLXContext context, int screen);
int __glXScreenFromContext(GLXContext context);

void __glXAddScreenFBConfigMapping(GLXFBConfig config, int screen);
void __glXRemoveScreenFBConfigMapping(GLXFBConfig config, int screen);
int __glXScreenFromFBConfig(GLXFBConfig config);

void __glXAddScreenDrawableMapping(GLXDrawable drawable, int screen);
void __glXRemoveScreenDrawableMapping(GLXDrawable drawable, int screen);
int __glXScreenFromDrawable(GLXDrawable drawable);

#endif /* __LIB_GLX_MAPPING_H */
