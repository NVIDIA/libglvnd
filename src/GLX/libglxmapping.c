/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include <X11/Xlibint.h>

#include <pthread.h>
#include <dlfcn.h>

#if defined(HASH_DEBUG)
# include <stdio.h>
#endif

#include "libglxmapping.h"
#include "libglxnoop.h"
#include "libglxthread.h"
#include "trace.h"

#define _GNU_SOURCE 1
#include <assert.h>
#include <stdio.h>

/*
 * This function queries each loaded vendor to determine if there is
 * a vendor-implemented dispatch function. The dispatch function
 * uses the vendor <-> API library ABI to determine the screen given
 * the parameters of the function and dispatch to the correct vendor's
 * implementation.
 */
__GLXextFuncPtr __glXGetGLXDispatchAddress(const GLubyte *procName)
{
    return NULL; // TODO;
}

__GLXvendorInfo *__glXLookupVendorByName(const char *vendorName)
{
    return NULL;
}

__GLXvendorInfo *__glXLookupVendorByScreen(Display *dpy, const int screen)
{
    return NULL; // TODO
}

const __GLXdispatchTableStatic *__glXGetStaticDispatch(Display *dpy, const int screen)
{
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    if (vendor) {
        assert(vendor->staticDispatch);
        return vendor->staticDispatch;
    } else {
        return __glXDispatchNoopPtr;
    }
}

void *__glXGetGLDispatch(Display *dpy, const int screen)
{
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    if (vendor) {
        assert(vendor->glDispatch);
        return vendor->glDispatch;
    } else {
        return NULL;
    }
}

__GLXdispatchTableDynamic *__glXGetDynDispatch(Display *dpy, const int screen)
{
    __GLXvendorInfo *vendor = __glXLookupVendorByScreen(dpy, screen);

    if (vendor) {
        assert(vendor->dynDispatch);
        return vendor->dynDispatch;
    } else {
        return NULL;
    }
}

static void AddScreenPointerMapping(void *ptr, int screen)
{
    // TODO
}


static void RemoveScreenPointerMapping(void *ptr, int screen)
{
    // TODO
}


static int ScreenFromPointer(void *ptr)
{
    // TODO
    return -1;
}


void __glXAddScreenContextMapping(GLXContext context, int screen)
{
    AddScreenPointerMapping(context, screen);
}


void __glXRemoveScreenContextMapping(GLXContext context, int screen)
{
    RemoveScreenPointerMapping(context, screen);
}


int __glXScreenFromContext(GLXContext context)
{
    return ScreenFromPointer(context);
}


void __glXAddScreenFBConfigMapping(GLXFBConfig config, int screen)
{
    AddScreenPointerMapping(config, screen);
}


void __glXRemoveScreenFBConfigMapping(GLXFBConfig config, int screen)
{
    RemoveScreenPointerMapping(config, screen);
}


int __glXScreenFromFBConfig(GLXFBConfig config)
{
    return ScreenFromPointer(config);
}




/****************************************************************************/
static void AddScreenXIDMapping(XID xid, int screen)
{
    // TODO
}


static void RemoveScreenXIDMapping(XID xid, int screen)
{
    // TODO
}


static int ScreenFromXID(Display *dpy, XID xid)
{
    return -1; // TODO
}


void __glXAddScreenDrawableMapping(GLXDrawable drawable, int screen)
{
    AddScreenXIDMapping(drawable, screen);
}


void __glXRemoveScreenDrawableMapping(GLXDrawable drawable, int screen)
{
    RemoveScreenXIDMapping(drawable, screen);
}


int __glXScreenFromDrawable(Display *dpy, GLXDrawable drawable)
{
    return ScreenFromXID(dpy, drawable);
}
