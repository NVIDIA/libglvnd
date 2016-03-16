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

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#include <assert.h>

#include "glvnd_list.h"
#include "x11glvnd.h"
#include "x11glvndproto.h"

const char *xglv_ext_name = XGLV_EXTENSION_NAME;
static XExtensionInfo *xglv_ext_info = NULL;

static /* const */ XExtensionHooks xglv_ext_hooks = {
    NULL,                               /* create_gc */
    NULL,                               /* copy_gc */
    NULL,                               /* flush_gc */
    NULL,                               /* free_gc */
    NULL,                               /* create_font */
    NULL,                               /* free_font */
    NULL,                               /* close_display */
    NULL,                               /* wire_to_event */
    NULL,                               /* event_to_wire */
    NULL,                               /* error */
    NULL,                               /* error_string */
};


static XEXT_GENERATE_FIND_DISPLAY(find_display, xglv_ext_info,
                                  (char *)xglv_ext_name,
                                  &xglv_ext_hooks,
                                  XGLV_NUM_EVENTS, NULL);

#define CHECK_EXTENSION(dpy, i, val)                \
    do {                                            \
        if (!XextHasExtension(i)) {                 \
            XMissingExtension(dpy, xglv_ext_name);  \
            UnlockDisplay(dpy);                     \
            SyncHandle();                           \
            return val;                             \
        }                                           \
   } while (0)


Bool XGLVQueryExtension(Display *dpy, int *event_base_return, int *error_base_return)
{
    XExtDisplayInfo *info = find_display(dpy);
    if (XextHasExtension(info)) {
        *event_base_return = info->codes->first_event;
        *error_base_return = info->codes->first_error;
        return True;
    } else {
        return False;
    }
}

Bool XGLVQueryVersion(Display *dpy, int *major, int *minor)
{
    XExtDisplayInfo *info = find_display(dpy);
    xglvQueryVersionReq *req;
    xglvQueryVersionReply rep;

    LockDisplay(dpy);

    CHECK_EXTENSION(dpy, info, False);

    GetReq(glvQueryVersion, req);

    req->reqType = info->codes->major_opcode;
    req->glvndReqType = X_glvQueryVersion;
    req->majorVersion = XGLV_EXT_MAJOR;
    req->minorVersion = XGLV_EXT_MINOR;

    if (!_XReply(dpy, (xReply*)&rep, 0, xTrue)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return False;
    }

    *major = rep.majorVersion;
    *minor = rep.minorVersion;
    UnlockDisplay(dpy);
    SyncHandle();
    return True;
}

/*
 * Returns the screen associated with this XID, or -1 if there was an error.
 */
int XGLVQueryXIDScreenMapping(
    Display *dpy,
    XID xid
)
{
    XExtDisplayInfo *info = find_display(dpy);
    xglvQueryXIDScreenMappingReq *req;
    xglvQueryXIDScreenMappingReply rep;

    LockDisplay(dpy);

    CHECK_EXTENSION(dpy, info, -1);

    GetReq(glvQueryXIDScreenMapping, req);

    req->reqType = info->codes->major_opcode;
    req->glvndReqType = X_glvQueryXIDScreenMapping;
    req->xid = xid;

    if (!_XReply(dpy, (xReply*)&rep, 0, xTrue)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return -1;
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return rep.screen;
}

/*
 * Returns the vendor associated with this screen, or NULL if there was an
 * error.
 */
char *XGLVQueryScreenVendorMapping(
    Display *dpy,
    int screen
)
{
    XExtDisplayInfo *info = find_display(dpy);
    xglvQueryScreenVendorMappingReq *req;
    xglvQueryScreenVendorMappingReply rep;
    size_t length, nbytes, slop;
    char *buf;

    LockDisplay(dpy);

    CHECK_EXTENSION(dpy, info, NULL);

    GetReq(glvQueryScreenVendorMapping, req);
    req->reqType = info->codes->major_opcode;
    req->glvndReqType = X_glvQueryScreenVendorMapping;
    req->screen = screen;

    if (!_XReply(dpy, (xReply*)&rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return NULL;
    }


    length = rep.length;
    nbytes = rep.n;

    if (!nbytes) {
        buf = NULL;
        assert(!length);
    } else {
        slop = nbytes & 3;
        buf = (char *)Xmalloc(nbytes);
        if (!buf) {
            _XEatData(dpy, length);
        } else {
            _XRead(dpy, (char *)buf, nbytes);
            if (slop) {
                _XEatData(dpy, 4-slop);
            }
        }
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return buf;
}

