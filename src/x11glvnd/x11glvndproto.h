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

#ifndef __X11GLVND_PROTO_H__
#define __X11GLVND_PROTO_H__

/*!
 * File describing protocol for the x11glvnd extension.
 */

#include "X11/Xmd.h"

#define XGLV_NUM_EVENTS 0
#define XGLV_NUM_ERRORS 0

#define XGLV_EXT_MAJOR 1
#define XGLV_EXT_MINOR 0

// TODO: X_glvQueryXIDVendorMapping?
#define X_glvQueryVersion               0
#define X_glvQueryXIDScreenMapping      1
#define X_glvQueryScreenVendorMapping   2
#define X_glvLastRequest  (X_glvQueryScreenVendorMapping+1)

#define GLVND_PAD(x) (((x)+3) & ~3)

#define GLVND_REPLY_HEADER(reply, len)          \
    (reply).type = X_Reply;                     \
    (reply).unused = 0;                         \
    (reply).sequenceNumber = client->sequence;  \
    (reply).length = len

/*
 * Following convenience macros are temporary and will be #undef'ed
 * after the protocol definitions.
 */

#define __GLV_REQ_PREAMBLE                    \
    CARD8 reqType;                            \
    CARD8 glvndReqType;                       \
    CARD16 length B16

#define __GLV_REPLY_PREAMBLE                  \
    BYTE type;                                \
    BYTE unused;                              \
    CARD16 sequenceNumber B16;                \
    CARD32 length B32

#define __GLV_DEFINE_REQ(name, layout)        \
    typedef struct xglv ## name ## ReqRec {   \
        __GLV_REQ_PREAMBLE;                   \
        layout                                \
    } xglv ## name ## Req;                    \
static const size_t sz_xglv ## name ## Req =  \
    GLVND_PAD(sizeof(xglv ## name ## Req))

#define __GLV_DEFINE_REPLY(name, layout)       \
    typedef struct xglv ## name ## ReplyRec {  \
        __GLV_REPLY_PREAMBLE;                  \
        layout                                 \
    } xglv ## name ## Reply;                   \
static const size_t sz_xglv ## name ## Reply = \
    32 // sizeof(xReply)

__GLV_DEFINE_REQ(QueryVersion,
    CARD32 majorVersion B32;
    CARD32 minorVersion B32;
);

__GLV_DEFINE_REPLY(QueryVersion,
    CARD32 majorVersion B32;
    CARD32 minorVersion B32;
    CARD32 padl4;
    CARD32 padl5;
    CARD32 padl6;
    CARD32 padl7;
    CARD32 padl8;
);

__GLV_DEFINE_REQ(QueryXIDScreenMapping,
    CARD32 xid B32;
);

__GLV_DEFINE_REPLY(QueryXIDScreenMapping,
    INT32  screen B32;
    CARD32 padl3;
    CARD32 padl4;
    CARD32 padl5;
    CARD32 padl6;
    CARD32 padl7;
    CARD32 padl8;
);

__GLV_DEFINE_REQ(QueryScreenVendorMapping,
    INT32 screen B32;
);

__GLV_DEFINE_REPLY(QueryScreenVendorMapping,
    CARD32 n    B32;
    CARD32 padl3;
    CARD32 padl4;
    CARD32 padl5;
    CARD32 padl6;
    CARD32 padl7;
    CARD32 padl8;
);

#undef __GLV_DEFINE_REQ
#undef __GLV_DEFINE_REPLY
#undef __GLV_REPLY_PREAMBLE
#undef __GLV_REQ_PREAMBLE

#endif
