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
 * Private dispatch table structure. This is used by GLdispatch for tracking
 * and updating dispatch tables.
 */
struct __GLdispatchTableRec {
    /*! Number of threads this dispatch is current on */
    int currentThreads;

    /*!
     * The number of dispatch table entries that have been populated. This is
     * used to update the table after generating new dispatch stubs.
     */
    int stubsPopulated;

    /*! Saved vendor library callbacks */
    __GLgetProcAddressCallback getProcAddress;
    void *getProcAddressParam;

    /*! The real dispatch table */
    struct _glapi_table *table;

    /*! List handle */
    struct glvnd_list entry;
};

#endif
