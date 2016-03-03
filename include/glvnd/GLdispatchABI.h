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
 * Thread-local implementation used by libglvnd. This is passed into the patch
 * function callback via the type parameter.
 *
 * For most architectures, the vendor library can ignore this parameter, since
 * it will always be the same value. It's used for systems like ARM, where the
 * stubs might be use the ARM or Thumb instruction sets.
 *
 * The stub type does not make any distinction between TLS and TSD stubs. The
 * entire purpose of entrypoint rewriting is to skip the dispatch table in
 * libGLdispatch.so, so it doesn't matter how that dispatch table is stored.
 */
enum {
    /*!
     * Indicates that the stubs aren't defined in assembly. For example, if the
     * dispatch stubs are written in C. Vendor libraries generally won't see
     * this value.
     */
    __GLDISPATCH_STUB_UNKNOWN,

    /*!
     * Used for stubs on x86 systems.
     */
    __GLDISPATCH_STUB_X86,

    /*!
     * Used for stubs on x86-64 systems.
     */
    __GLDISPATCH_STUB_X86_64,

    /*!
     * Used for stubs on ARMv7, using the Thumb instruction set.
     */
    __GLDISPATCH_STUB_ARMV7_THUMB,

    /*!
     * Used for stubs on ARMv7, using the normal ARM instruction set.
     */
    __GLDISPATCH_STUB_ARMV7_ARM
};

/*!
 * A callback function called by the vendor library to fetch the address of an
 * entrypoint.
 *
 * The function returns two pointers, one writable and one executable. The two
 * pointers may or may not be the same virtual address, but they will both be
 * mappings of the same physical memory.
 *
 * The vendor library should write its entrypoint to the address returned by
 * \p writePtr, but should use the address from \p execPtr for things like
 * calculating PC-relative offsets.
 *
 * Note that if this function fails, then the vendor library can still try to
 * patch other entrypoints.
 *
 * Note that on ARM, the low-order bit of both \c execPtr and \p writePtr will
 * be zero, even if the stub uses the thumb instruction set. The vendor library
 * should use the \c type parameter of \c initiatePatch to determine which
 * instruction set to use.
 *
 * \param funcName The function name.
 * \param[out] writePtr The pointer that the vendor library can write to.
 * \param[out] execPtr The pointer to the executable code.
 * \return GL_TRUE if the entrypoint exists, or GL_FALSE if it doesn't.
 */
typedef GLboolean (*DispatchPatchLookupStubOffset)(const char *funcName,
        void **writePtr, const void **execPtr);

typedef struct __GLdispatchPatchCallbacksRec {
    /*!
     * Checks to see if the vendor library supports patching the given stub
     * type and size.
     *
     * \param type The type of entrypoints. This will be a one of the
     * __GLDISPATCH_STUB_* values.
     * \param stubSize The maximum size of the stub that the vendor library can
     * write, in bytes.
     * \param lookupStubOffset A callback into libglvnd to look up the address
     * of each entrypoint.
     */
    GLboolean (* isPatchSupported)(int type, int stubSize);

    /*!
     * Called by libglvnd to request that a vendor library patch its top-level
     * entrypoints.
     *
     * The vendor library should use the \p lookupStubOffset callback to find
     * the addresses of each entrypoint.
     *
     * This function may be called more than once to patch multiple sets of
     * entrypoints. For example, depending on how they're built, libOpenGL.so
     * or libGL.so may have their own entrypoints that are separate functions
     * from the ones in libGLdispatch.
     *
     * Note that during this call is the only time that the entrypoints can be
     * modified. After the call to \c initiatePatch returns, the vendor library
     * should treat the entrypoints as read-only.
     *
     * \param type The type of entrypoints. This will be a one of the
     * __GLDISPATCH_STUB_* values.
     * \param stubSize The maximum size of the stub that the vendor library can
     * write, in bytes.
     * \param lookupStubOffset A callback into libglvnd to look up the address
     * of each entrypoint.
     *
     * \return GL_TRUE if the vendor library supports patching with this type
     * and size.
     */
    GLboolean (*initiatePatch)(int type,
                               int stubSize,
                               DispatchPatchLookupStubOffset lookupStubOffset);

    /*!
     * (OPTIONAL) Called by libglvnd to notify the current vendor that it no
     * longer owns the top-level entrypoints.
     *
     * Libglvnd will take care of the restoring the entrypoints back to their
     * original state. The vendor library must not try to modify them.
     */
    void (*releasePatch)(void);

    /*!
     * (OPTIONAL) Called at the start of window-system functions (GLX and EGL).
     * This callback allows vendor libraries to perform any per-thread
     * initialization.
     *
     * This is basically a workaround for broken applications. A lot of apps
     * will make one or more invalid GLX/EGL calls on a thread (often including
     * a MakeCurrent with invalid parameters), and then will try to call an
     * OpenGL function.
     *
     * A non-libglvnd-based driver would be able to initialize any thread state
     * even on a bogus GLX call, but with libglvnd, those calls wouldn't get
     * past libGLX.
     *
     * This function is optional. If it's \c NULL, then libGLdispatch will
     * simply ignore it.
     *
     * \note This function may be called concurrently from multiple threads.
     */
    void (*threadAttach)(void);
} __GLdispatchPatchCallbacks;

#if defined(__cplusplus)
}
#endif

#endif // __GL_DISPATCH_ABI_H
