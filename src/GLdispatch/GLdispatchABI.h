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

#include <GL/gl.h>

#if !defined(__GL_DISPATCH_ABI_H)
#define __GL_DISPATCH_ABI_H

/*!
 * \defgroup gldispatchabi GL dispatching ABI
 *
 * This is not a complete ABI, but rather a fragment common to the libEGL and
 * libGLX ABIs.  Changes to this file should be accompanied by a version bump to
 * these client ABIs.
 */

/*!
 * This opaque structure describes the core GL dispatch table.
 */
typedef struct __GLdispatchTableRec __GLdispatchTable;

typedef void (*__GLdispatchProc)(void);

typedef void *(*__GLgetProcAddressCallback)(const GLubyte *procName,
                                            void *vendorData);

typedef void *(*__GLgetProcAddressCallback)(const GLubyte *procName,
                                            void *vendorData);

typedef GLboolean (*__GLgetDispatchProtoCallback)(const GLubyte *procName,
                                                  char ***function_names,
                                                  char **parameter_signature);

typedef void (*__GLdestroyVendorDataCallback)(void *vendorData);



typedef struct __GLdispatchExportsRec {
    /************************************************************************
     * When a context is current, for performance reasons it may be desirable
     * for a vendor to use different entrypoints for that context depending on
     * the current GL state. The following routines allow a vendor to create and
     * manage auxiliary dispatch tables for this purpose.
     ************************************************************************/

    /*!
     * This retrieves the current core GL dispatch table.
     */
    __GLdispatchTable    *(*getCurrentGLDispatch)(void);

    /*!
     * This retrieves the top-level GL dispatch table for the current vendor.
     * This must always be defined for the lifetime of the vendor library.
     */
    __GLdispatchTable     *(*getTopLevelDispatch)(void);

    /*!
     * This creates an auxiliary core GL dispatch table using the given
     * vendor-specific callbacks and data. This data will be passed to the
     * getProcAddress callback during dispatch table construction and can be
     * used to discriminate between different flavors of entrypoints in the
     * vendor.
     * XXX: is the getProcAddress callback method too slow? Should we have
     * a way for vendor libraries to declare fixed tables at startup that
     * can be read quickly?
     */
    __GLdispatchTable   *(*createGLDispatch)(
        __GLgetProcAddressCallback getProcAddress,
        __GLgetDispatchProtoCallback getDispatchProto,
        __GLdestroyVendorDataCallback destroyVendorData,
        void *vendorData
    );

    /*!
     * This retrieves the offset into the GL dispatch table for the given
     * function name, or -1 if the function is not found.
     * If a valid offset is returned, the offset is valid for all dispatch
     * tables for the lifetime of the API library.
     * XXX: should there be a way for vendor libraries to pre-load procs
     * they care about?
     */
    GLint (*getGLDispatchOffset)(const GLubyte *procName);

    /*!
     * This sets the given entry in the GL dispatch table to the function
     * address pointed to by addr.
     */
    void (*setGLDispatchEntry)(__GLdispatchTable *table,
                               GLint offset,
                               __GLdispatchProc addr);

    /*!
     * This makes the given GL dispatch table current. Note this operation
     * is only valid when there is a GL context owned by the vendor which
     * is current.
     */
    void (*makeGLDispatchCurrent)(__GLdispatchTable *table);

    /*!
     * This destroys the given GL dispatch table, and returns GL_TRUE on
     * success. Note it is an error to attempt to destroy the top-level
     * dispatch.
     */
    GLboolean (*destroyGLDispatch)(__GLdispatchTable *table);

} __GLdispatchExports;


#endif // __GL_DISPATCH_ABI_H
