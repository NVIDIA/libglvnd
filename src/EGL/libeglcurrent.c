#include "libeglcurrent.h"

#include "glvnd_pthread.h"
#include "lkdhash.h"

static void OnDispatchThreadDestroyed(__GLdispatchThreadState *state);
static void OnThreadDestroyed(void *data);

static __EGLThreadAPIState *CreateThreadState(void);
static void DestroyThreadState(__EGLThreadAPIState *threadState);

/**
 * A list of current __EGLdispatchThreadState structures. This is used so that we can
 * clean up at process termination or after a fork.
 */
static struct glvnd_list currentAPIStateList;
static struct glvnd_list currentThreadStateList;
static glvnd_mutex_t currentStateListMutex = PTHREAD_MUTEX_INITIALIZER;
static glvnd_key_t threadStateKey;

EGLenum __eglQueryAPI(void)
{
    __EGLThreadAPIState *state = __eglGetCurrentThreadAPIState(EGL_FALSE);
    if (state != NULL) {
        return state->currentClientApi;
    } else {
        // FIXME: If none of the vendor libraries support GLES, then this
        // should be EGL_NONE.
        return EGL_OPENGL_ES_API;
    }
}

__EGLvendorInfo *__eglGetCurrentVendor(void)
{
    __EGLdispatchThreadState *state = __eglGetCurrentAPIState();
    if (state != NULL) {
        return state->currentVendor;
    } else {
        return NULL;
    }
}

EGLContext __eglGetCurrentContext(void)
{
    __EGLdispatchThreadState *state = __eglGetCurrentAPIState();
    if (state != NULL) {
        return state->currentContext;
    } else {
        return EGL_NO_CONTEXT;
    }
}

EGLDisplay __eglGetCurrentDisplay(void)
{
    __EGLdispatchThreadState *state = __eglGetCurrentAPIState();
    if (state != NULL && state->currentDisplay != NULL) {
        return state->currentDisplay->dpy;
    } else {
        return EGL_NO_DISPLAY;
    }
}

EGLSurface __eglGetCurrentSurface(EGLint readDraw)
{
    __EGLdispatchThreadState *state = __eglGetCurrentAPIState();
    if (state != NULL) {
        if (readDraw == EGL_DRAW) {
            return state->currentDraw;
        } else if (readDraw == EGL_READ) {
            return state->currentRead;
        } else {
            return EGL_NO_SURFACE;
        }
    } else {
        return EGL_NO_SURFACE;
    }
}

void __eglCurrentInit(void)
{
    glvnd_list_init(&currentAPIStateList);
    glvnd_list_init(&currentThreadStateList);
    __glvndPthreadFuncs.key_create(&threadStateKey, OnThreadDestroyed);
}

void __eglCurrentTeardown(EGLBoolean doReset)
{
    while (!glvnd_list_is_empty(&currentAPIStateList)) {
        __EGLdispatchThreadState *apiState = glvnd_list_first_entry(
                &currentAPIStateList, __EGLdispatchThreadState, entry);
        __eglDestroyAPIState(apiState);
    }

    while (!glvnd_list_is_empty(&currentThreadStateList)) {
        __EGLThreadAPIState *threadState = glvnd_list_first_entry(
                &currentThreadStateList, __EGLThreadAPIState, entry);
        DestroyThreadState(threadState);
    }

    if (doReset) {
        __glvndPthreadFuncs.mutex_init(&currentStateListMutex, NULL);
    }
}

__EGLThreadAPIState *CreateThreadState(void)
{
    __EGLThreadAPIState *threadState = calloc(1, sizeof(__EGLThreadAPIState));
    if (threadState == NULL) {
        return NULL;
    }

    threadState->lastError = EGL_SUCCESS;
    threadState->lastVendor = NULL;

    // TODO: If no vendor library supports GLES, then we should initialize this
    // to EGL_NONE.
    threadState->currentClientApi = EGL_OPENGL_ES_API;

    __glvndPthreadFuncs.mutex_lock(&currentStateListMutex);
    glvnd_list_add(&threadState->entry, &currentThreadStateList);
    __glvndPthreadFuncs.mutex_unlock(&currentStateListMutex);

    __glvndPthreadFuncs.setspecific(threadStateKey, threadState);
    return threadState;
}

__EGLThreadAPIState *__eglGetCurrentThreadAPIState(EGLBoolean create)
{
    __EGLThreadAPIState *threadState = (__EGLThreadAPIState *) __glvndPthreadFuncs.getspecific(threadStateKey);
    if (threadState == NULL && create) {
        threadState = CreateThreadState();
    }
    return threadState;
}

void DestroyThreadState(__EGLThreadAPIState *threadState)
{
    if (threadState != NULL) {
        __glvndPthreadFuncs.mutex_lock(&currentStateListMutex);
        glvnd_list_del(&threadState->entry);
        __glvndPthreadFuncs.mutex_unlock(&currentStateListMutex);

        free(threadState);
    }
}

void __eglDestroyCurrentThreadAPIState(void)
{
    __EGLThreadAPIState *threadState = __glvndPthreadFuncs.getspecific(threadStateKey);
    if (threadState != NULL) {
        __glvndPthreadFuncs.setspecific(threadStateKey, NULL);
        DestroyThreadState(threadState);
    }
}

void OnThreadDestroyed(void *data)
{
    __EGLThreadAPIState *threadState = (__EGLThreadAPIState *) data;
    DestroyThreadState(threadState);
}

__EGLdispatchThreadState *__eglCreateAPIState(void)
{
    __EGLdispatchThreadState *apiState = calloc(1, sizeof(__EGLdispatchThreadState));
    if (apiState == NULL) {
        return NULL;
    }

    apiState->glas.tag = GLDISPATCH_API_EGL;
    apiState->glas.threadDestroyedCallback = OnDispatchThreadDestroyed;

    apiState->currentDisplay = NULL;
    apiState->currentDraw = EGL_NO_SURFACE;
    apiState->currentRead = EGL_NO_SURFACE;
    apiState->currentContext = EGL_NO_CONTEXT;
    apiState->currentVendor = NULL;

    __glvndPthreadFuncs.mutex_lock(&currentStateListMutex);
    glvnd_list_add(&apiState->entry, &currentAPIStateList);
    __glvndPthreadFuncs.mutex_unlock(&currentStateListMutex);

    return apiState;
}

void __eglDestroyAPIState(__EGLdispatchThreadState *apiState)
{
    if (apiState != NULL) {
        __glvndPthreadFuncs.mutex_lock(&currentStateListMutex);
        glvnd_list_del(&apiState->entry);
        __glvndPthreadFuncs.mutex_unlock(&currentStateListMutex);

        free(apiState);
    }
}

void OnDispatchThreadDestroyed(__GLdispatchThreadState *state)
{
    __EGLdispatchThreadState *eglState = (__EGLdispatchThreadState *) state;
    __eglDestroyAPIState(eglState);
}

