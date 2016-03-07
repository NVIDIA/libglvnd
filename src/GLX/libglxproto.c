/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
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

#include "libglxproto.h"

#include <GL/glx.h>
#include <GL/glxproto.h>

/*!
 * Reads a reply from the server, including any additional data.
 *
 * This function will read a reply from the server, and it will optionally
 * allocate a buffer and read any variable-length data in the reply.
 *
 * If \c replyData is not \c NULL and the reply contains additional data, then
 * this function will allocate a buffer and read the data into it.
 *
 * If \c replyData is \c NULL, then the caller must read any data in the reply.
 *
 * If the last request generates an error, then it will return the error code,
 * but it will not call the normal X error handler.
 *
 * \param dpyInfo The display connection.
 * \param[out] reply Returns the reply structure.
 * \param[out] replyData If not \c NULL, returns any additional reply data.
 * \return \c Success on a successful reply. If the server sent back an error,
 * then the error code is returned. If something else fails, then -1 is
 * returned.
 */
static Status ReadReply(__GLXdisplayInfo *dpyInfo, xReply *reply, void **replyData)
{
    Display *dpy = dpyInfo->dpy;
    _XAsyncHandler async;
    _XAsyncErrorState state = {};
    Status error = Success;

    state.min_sequence_number = state.max_sequence_number = dpy->request;
    state.major_opcode = dpyInfo->glxMajorOpcode;
    async.next = dpy->async_handlers;
    async.handler = _XAsyncErrorHandler;
    async.data = (XPointer) &state;
    dpy->async_handlers = &async;

    if (!_XReply(dpy, reply, 0, False)) {
        error = -1;
    }
    DeqAsyncHandler(dpy, &async);

    if (state.error_count) {
        error = state.last_error_received;
        if (error == Success) {
            assert(error != Success);
            error = -1;
        }
    }

    if (replyData != NULL) {
        void *data = NULL;
        if (error == Success && reply->generic.length > 0) {
            int length = reply->generic.length * 4;
            data = malloc(length);
            if (data != NULL) {
                _XRead(dpyInfo->dpy, (char *) data, length);
            } else {
                _XEatData(dpyInfo->dpy, length);
                error = -1;
            }
        }
        *replyData = data;;
    }

    return error;
}

char *__glXQueryServerString(__GLXdisplayInfo *dpyInfo, int screen, int name)
{
    Display *dpy = dpyInfo->dpy;
    xGLXQueryServerStringReq *req;
    xGLXSingleReply rep;
    char *ret = NULL;

    if (!dpyInfo->glxSupported) {
        return NULL;
    }

    LockDisplay(dpy);

    GetReq(GLXQueryServerString, req);
    req->reqType = dpyInfo->glxMajorOpcode;
    req->glxCode = X_GLXQueryServerString;
    req->screen = screen;
    req->name = name;

    ReadReply(dpyInfo, (xReply *) &rep, (void **) &ret);

    UnlockDisplay(dpy);
    SyncHandle();

    return ret;
}
int __glXGetDrawableScreen(__GLXdisplayInfo *dpyInfo, GLXDrawable drawable)
{
    Display *dpy = dpyInfo->dpy;
    xGLXGetDrawableAttributesReq *req;
    xGLXGetDrawableAttributesReply rep;
    int *attribs = NULL;
    Status st;

    if (drawable == None) {
        return -1;
    }
    if (!dpyInfo->glxSupported) {
        return 0;
    }

    LockDisplay(dpy);

    GetReq(GLXGetDrawableAttributes, req);
    req->reqType = dpyInfo->glxMajorOpcode;
    req->glxCode = X_GLXGetDrawableAttributes;
    req->drawable = drawable;

    st = ReadReply(dpyInfo, (xReply *) &rep, (void **) &attribs);

    UnlockDisplay(dpy);
    SyncHandle();

    if (st == Success) {
        int screen = 0;
        unsigned int i;

        if (attribs != NULL) {
            for (i=0; i<rep.numAttribs; i++) {
                if (attribs[i * 2] == GLX_SCREEN) {
                    screen = attribs[i * 2 + 1];
                    break;
                }
            }
            free(attribs);
        }
        return screen;
    } else {
        return -1;
    }
}

