/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <semaphore.h>
#include <GL/gl.h>

#include <GLdispatch.h>

#define VENDOR_COUNT 2
#define THREAD_COUNT 4
#define CALL_COUNT 1

typedef struct {
    int vendorID;
    __GLdispatchTable *dispatch;
} VendorInfo;

typedef struct {
    __GLdispatchThreadState dispatchThreadState;

    pthread_t thread;
    VendorInfo *vendor;

    int callCountStatic;
    int callCountEarly;
    int callCountLate;
    GLboolean destroyed;
} ThreadState;

typedef void (* pfn_glVertex3fv) (const GLfloat *v);

static sem_t mainSemaphore;
static sem_t threadSemaphore;
static pfn_glVertex3fv ptr_glVertex3fv = NULL;
static pfn_glVertex3fv ptr_glTestFuncEarly = NULL;
static pfn_glVertex3fv ptr_glTestFuncLate = NULL;

static ThreadState *GetCurrentThreadState(void)
{
    __GLdispatchThreadState *ts = __glDispatchGetCurrentThreadState();
    if (ts == NULL) {
        printf("__glDispatchGetCurrentThreadState failed\n");
    }
    return (ThreadState *) ts;
}

static void dummy_glVertex3fv(const GLfloat *v)
{
    ThreadState *ts = GetCurrentThreadState();
    ts->callCountStatic++;
}

static void glTestFuncEarlyDUMMY(const GLfloat *v)
{
    ThreadState *ts = GetCurrentThreadState();
    ts->callCountEarly++;
}
static void glTestFuncLateDUMMY(const GLfloat *v)
{
    ThreadState *ts = GetCurrentThreadState();
    ts->callCountLate++;
}

static void *VendorGetProcAddressCallback(const char *procName, void *param)
{
    if (strcmp(procName, "glVertex3fv") == 0) {
        return dummy_glVertex3fv;
    } else if (strcmp(procName, "glTestFuncEarlyDUMMY") == 0) {
        return glTestFuncEarlyDUMMY;
    } else if (strcmp(procName, "glTestFuncLateDUMMY") == 0) {
        return glTestFuncLateDUMMY;
    } else {
        return NULL;
    }
}

static void ThreadDestroyedCallback(__GLdispatchThreadState *dispatchThreadState)
{
    ThreadState *ts = (ThreadState *) dispatchThreadState;
    ts->destroyed = GL_TRUE;
}

static void *ThreadProc(void *param)
{
    ThreadState *ts = (ThreadState *) param;
    int i;

    ts->dispatchThreadState.threadDestroyedCallback = ThreadDestroyedCallback;

    if (!__glDispatchMakeCurrent(&ts->dispatchThreadState,
                ts->vendor->dispatch, ts->vendor->vendorID,
                NULL)) {
        printf("__glDispatchMakeCurrent failed\n");
        exit(1);
    }

    // Notify the main thread that we're ready.
    if (sem_post(&mainSemaphore) != 0) {
        printf("sem_post failed\n");
        exit(1);
    }

    // Wait for the main thread to finish calling __glDispatchGetProcAddress.
    if (sem_wait(&threadSemaphore) != 0) {
        printf("sem_wait failed\n");
        exit(1);
    }

    for (i=0; i<CALL_COUNT; i++) {
        ptr_glVertex3fv(NULL);
        if (ptr_glTestFuncEarly != NULL) {
            ptr_glTestFuncEarly(NULL);
        }
        if (ptr_glTestFuncLate != NULL) {
            ptr_glTestFuncLate(NULL);
        }
    }

    return NULL;
}

int main(int argc, char **argv)
{
    VendorInfo vendors[VENDOR_COUNT] = {};
    ThreadState threads[THREAD_COUNT] = {};
    int result, i;

    if (sem_init(&mainSemaphore, 0, 0) != 0) {
        printf("sem_init failed\n");
        return 1;
    }

    if (sem_init(&threadSemaphore, 0, 0) != 0) {
        printf("sem_init failed\n");
        return 1;
    }

    __glDispatchInit();

    ptr_glVertex3fv = (pfn_glVertex3fv) __glDispatchGetProcAddress("glVertex3fv");
    if (ptr_glVertex3fv == NULL) {
        printf("__glDispatchGetProcAddress(glVertex3fv) failed\n");
        return 1;
    }

#if defined(USE_DISPATCH_ASM)
    ptr_glTestFuncEarly = (pfn_glVertex3fv) __glDispatchGetProcAddress("glTestFuncEarlyDUMMY");
    if (ptr_glTestFuncEarly == NULL) {
        printf("__glDispatchGetProcAddress(glTestFuncEarlyDUMMY) failed\n");
        return 1;
    }
#endif

    // Create some dummy vendors and dispatch tables.
    for (i=0; i<VENDOR_COUNT; i++) {
        vendors[i].vendorID = __glDispatchNewVendorID();
        vendors[i].dispatch = __glDispatchCreateTable(VendorGetProcAddressCallback, &vendors[i]);
        if (vendors[i].dispatch == NULL) {
            return 1;
        }
    }

    // Start the worker threads.
    for (i=0; i<THREAD_COUNT; i++) {
        threads[i].vendor = &vendors[i % VENDOR_COUNT];

        if (pthread_create(&threads[i].thread, NULL, ThreadProc, &threads[i]) != 0) {
            printf("Failed to create thread\n");
            return 1;
        }
    }

    // Wait for each thread to be ready. After this, each thread will have
    // called __glDispatchMakeCurrent, and is waiting for the main thread to
    // tell it to proceed.
    for (i=0; i<THREAD_COUNT; i++) {
        if (sem_wait(&mainSemaphore) != 0) {
            printf("sem_wait failed\n");
            exit(1);
        }
    }

#if defined(USE_DISPATCH_ASM)
    // Generate another GL function. This tests whether libGLdispatch will
    // correctly update existing dispatch tables that are current to some
    // thread.
    ptr_glTestFuncLate = (pfn_glVertex3fv) __glDispatchGetProcAddress("glTestFuncLateDUMMY");
    if (ptr_glTestFuncLate == NULL) {
        printf("__glDispatchGetProcAddress(glTestFuncLateDUMMY) failed\n");
        return 1;
    }
#endif

    // Wake up the threads and let them continue. The threads will try calling
    // each of the GL functions.
    for (i=0; i<THREAD_COUNT; i++) {
        if (sem_post(&threadSemaphore) != 0) {
            printf("sem_post failed\n");
            exit(1);
        }
    }

    // Wait for the threads to finish.
    for (i=0; i<THREAD_COUNT; i++) {
        pthread_join(threads[i].thread, NULL);
    }

    // Check the results.
    result = 0;
    for (i=0; i<THREAD_COUNT; i++) {
        if (threads[i].callCountStatic != CALL_COUNT) {
            printf("Thread %d: Static call count is wrong: %d\n", i, threads[i].callCountStatic);
            result = 1;
        }
        if (ptr_glTestFuncEarly != NULL) {
            if (threads[i].callCountEarly != CALL_COUNT) {
                printf("Thread %d: Early call count is wrong: %d\n", i, threads[i].callCountEarly);
                result = 1;
            }
        }
        if (ptr_glTestFuncLate != NULL) {
            if (threads[i].callCountLate != CALL_COUNT) {
                printf("Thread %d: Late call count is wrong: %d\n", i, threads[i].callCountLate);
                result = 1;
            }
        }
        if (!threads[i].destroyed) {
            printf("Thread %d: Destroy callback was not called\n", i);
            result = 1;
        }
    }

    for (i=0; i<VENDOR_COUNT; i++) {
        if (vendors[i].dispatch != NULL) {
            __glDispatchDestroyTable(vendors[i].dispatch);
        }
    }

    sem_destroy(&mainSemaphore);
    sem_destroy(&threadSemaphore);

    return result;
}

