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

#if defined(__cplusplus)
extern "C" {
#endif

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
                                            int isClientAPI);

/*
 * Thread-local implementation used by libglvnd.  This is passed into
 * the patch function callback via the type parameter.
 */
enum {
    __GLDISPATCH_STUB_X86_TLS,
    __GLDISPATCH_STUB_X86_64_TLS,
    __GLDISPATCH_STUB_X86_TSD,
    __GLDISPATCH_STUB_PURE_C,
    __GLDISPATCH_STUB_X86_64_TSD,
    __GLDISPATCH_STUB_NUM_TYPES
};

typedef struct __GLdispatchPatchCallbacksRec {
    /*
     * Called by libglvnd to request that a vendor library patch its top-level
     * entrypoints.  The vendor should return GL_TRUE if patching is supported
     * with this type and stub size, or GL_FALSE otherwise.  If this is the first
     * time libglvnd calls into the vendor with the given stubGeneration argument,
     * the vendor is expected to set the boolean pointed to by needOffsets to
     * GL_TRUE; otherwise, it should be set to GL_FALSE.
     */
    GLboolean (*initiatePatch)(int type,
                               int stubSize,
                               GLint64 stubGeneration,
                               GLboolean *needOffsets);

    /*
     * Hook by which the vendor library may request stub offsets if it set
     * *needOffsets == GL_TRUE above.
     */
    void (*getOffsetHook)(void *(*lookupStubOffset)(const char *funcName));

    /*
     * Called by libglvnd to finish the initial top-level entrypoint patch.
     * libglvnd must have called the __GLdispatchInitiatePatch callback first!
     * After this function is called, the vendor "owns" the top-level
     * entrypoints and may change them at will until GLdispatch calls the
     * releasePatch callback below.
     */
    void (*finalizePatch)(void);

    /*
     * Called by libglvnd to notify the current vendor that it no longer owns
     * the top-level entrypoints.
     */
    void (*releasePatch)(void);
} __GLdispatchPatchCallbacks;

#if defined(__cplusplus)
}
#endif

#endif // __GL_DISPATCH_ABI_H
