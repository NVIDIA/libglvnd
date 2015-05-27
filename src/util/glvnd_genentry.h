#ifndef GLVND_GENENTRY_H
#define GLVND_GENENTRY_H

/**
 * \file
 *
 * Functions for generating simple entrypoints that only jump to another
 * function.
 *
 * Unlike the functions generated from libGLdispatch, these functions don't use
 * any per-thread state. Each one has exactly one function that it jumps to.
 *
 * These are used to generate entrypoints for GLX extension functions when we
 * haven't loaded a vendor library that supports them yet. We can return a
 * generated entrypoint function to the app, and then later on, after we load a
 * vendor library, we can find and plug in the actual dispatch functions.
 *
 * Each entrypoint starts with a default dispatch function, which currently
 * does nothing and returns NULL.
 *
 * Note that these functions are not thread-safe. The caller must ensure that
 * only one thread at a time calls \c glvndGenerateEntrypoint or
 * \c glvndUpdateEntrypoints.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * A typedef used for a generic entrypoint or dispatch stub.
 *
 * Note that this type is identical to __GLXextFuncPtr,
 * __eglMustCastToProperFunctionPointerType, and __GLdispatchProc.
 */
typedef void (* GLVNDentrypointStub)(void);

/**
 * A callback used by \c glvndUpdateEntrypoints.
 *
 * It should return a pointer to a function, which will be plugged into the
 * entrypoint.
 *
 * If no matching function is found, then the callback should return \c NULL.
 *
 * \param procName The name of the function to look up.
 * \param param The parameter passed to \c glvndUpdateEntrypoints.
 * \return A function pointer, or NULL.
 */
typedef GLVNDentrypointStub (* GLVNDentrypointUpdateCallback) (const char *procName, void *param);


/**
 * Generates an entrypoint for a function.
 *
 * Multiple calls for the same name will return the same function.
 *
 * \param procName The name of the function.
 * \return A newly-generated entrypoint function, or \c NULL on error.
 */
GLVNDentrypointStub glvndGenerateEntrypoint(const char *procName);

/**
 * Frees any memory allocated for the generated entrypoints.
 */
void glvndFreeEntrypoints(void);

/**
 * Goes through the list of entrypoints and calls \p callback for each
 * entrypoint that doesn't have a function assigned to it yet.
 *
 * If the callback returns a non-NULL value, then that function will be
 * assigned to the entrypoint.
 *
 * If the callback returns \c NULL, then the entrypoint is not modified.
 *
 * \param callback A callback to look up the functions for each entrypoint.
 * \param param An arbitrary value passed to \p callback.
 */
void glvndUpdateEntrypoints(GLVNDentrypointUpdateCallback callback, void *param);

#if defined(__cplusplus)
}
#endif

#endif // GLVND_GENENTRY_H
