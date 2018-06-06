#ifndef EGLDISPATCHSTUBS_H
#define EGLDISPATCHSTUBS_H

#include "glvnd/libeglabi.h"
#include "compiler.h"

// These variables are all generated along with the dispatch stubs.
extern const int __EGL_DISPATCH_FUNC_COUNT;
extern const char * const __EGL_DISPATCH_FUNC_NAMES[];
extern int __EGL_DISPATCH_FUNC_INDICES[];
extern const __eglMustCastToProperFunctionPointerType __EGL_DISPATCH_FUNCS[];

void __eglInitDispatchStubs(const __EGLapiExports *exportsTable);

// Helper functions used by the generated stubs.
__eglMustCastToProperFunctionPointerType __eglDispatchFetchByDisplay(EGLDisplay dpy, int index);
__eglMustCastToProperFunctionPointerType __eglDispatchFetchByDevice(EGLDeviceEXT dpy, int index);
__eglMustCastToProperFunctionPointerType __eglDispatchFetchByCurrent(int index);

#endif // EGLDISPATCHSTUBS_H
