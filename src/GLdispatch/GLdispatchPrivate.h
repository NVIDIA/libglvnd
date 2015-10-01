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

#ifndef __GL_DISPATCH_PRIVATE_H__
#define __GL_DISPATCH_PRIVATE_H__

#include "GLdispatch.h"
#include "glapi.h"
#include "glvnd_list.h"
#include "entry.h"
#include "utils_misc.h"

/*!
 * XXX: Any changes to the internal mapi enum should be accompanied by an ABI
 * update, and vice versa.
 */
#define TLS_TYPE_CHECK(x) STATIC_ASSERT((int)__GLDISPATCH_STUB_ ## x == (int)ENTRY_ ## x)

static inline void UNUSED __unused_tls_type_check(void)
{
    TLS_TYPE_CHECK(X86_TLS);
    TLS_TYPE_CHECK(X86_64_TLS);
    TLS_TYPE_CHECK(X86_TSD);
    TLS_TYPE_CHECK(PURE_C);
    TLS_TYPE_CHECK(X86_64_TSD);
    TLS_TYPE_CHECK(ARMV7_THUMB_TSD);
    TLS_TYPE_CHECK(NUM_TYPES);
}

#undef TLS_TYPE_CHECK

/*!
 * Private dispatch table structure. This is used by GLdispatch for tracking
 * and updating dispatch tables.
 */
struct __GLdispatchTableRec {
    /*! Number of threads this dispatch is current on */
    int currentThreads;

    /*! Generation number for tracking whether this needs fixup */
    int generation;

    /*! Saved vendor library callbacks */
    __GLgetProcAddressCallback getProcAddress;
    void *getProcAddressParam;

    /*! The real dispatch table */
    struct _glapi_table *table;

    /*! List handle */
    struct glvnd_list entry;
};

#endif
