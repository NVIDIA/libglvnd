/*
 * Copyright (c) 2017, NVIDIA CORPORATION.
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
#include <GL/gl.h>

#include <GLdispatch.h>

#include "dummy/patchentrypoints.h"

#define DUMMY_VENDOR_COUNT 3
#define NUM_GLDISPATCH_CALLS 2
static const char *GENERATED_FUNCTION_NAME = "glDummyTestGLVND";

enum {
    CALL_INDEX_STATIC,
    CALL_INDEX_GENERATED,
    CALL_INDEX_STATIC_PATCH,
    CALL_INDEX_GENERATED_PATCH,
    CALL_INDEX_COUNT
};

typedef void (* pfn_glVertex3fv) (const GLfloat *v);

typedef struct DummyVendorLibRec {
    pfn_glVertex3fv vertexProc;
    pfn_glVertex3fv testProc;
    __GLgetProcAddressCallback getProcCallback;

    __GLdispatchThreadState threadState;
    __GLdispatchTable *dispatch;
    int vendorID;
    const __GLdispatchPatchCallbacks *patchCallbacksPtr;
    __GLdispatchPatchCallbacks patchCallbacks;

    int callCounts[CALL_INDEX_COUNT];
} DummyVendorLib;

static void InitDummyVendors(void);
static void CleanupDummyVendors(void);

static GLboolean TestDispatch(int vendorIndex,
        GLboolean testStatic, GLboolean testGenerated);

static void *common_getProcAddressCallback(const char *procName, void *param, int vendorIndex);
static GLboolean common_InitiatePatch(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset, int vendorIndex);

static void *dummy0_getProcAddressCallback(const char *procName, void *param);
static void dummy0_glVertex3fv(const GLfloat *v);
static void dummy0_glDummyTestProc(const GLfloat *v);
static GLboolean dummy0_InitiatePatch(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset);

static void *dummy1_getProcAddressCallback(const char *procName, void *param);
static void dummy1_glVertex3fv(const GLfloat *v);
static void dummy1_glDummyTestProc(const GLfloat *v);
static GLboolean dummy1_InitiatePatch(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset);

static void *dummy2_getProcAddressCallback(const char *procName, void *param);
static void dummy2_glVertex3fv(const GLfloat *v);
static void dummy2_glDummyTestProc(const GLfloat *v);

static DummyVendorLib dummyVendors[DUMMY_VENDOR_COUNT] = {
    { dummy0_glVertex3fv, dummy0_glDummyTestProc, dummy0_getProcAddressCallback },
    { dummy1_glVertex3fv, dummy1_glDummyTestProc, dummy1_getProcAddressCallback },
    { dummy2_glVertex3fv, dummy2_glDummyTestProc, dummy2_getProcAddressCallback },
};

static pfn_glVertex3fv ptr_glVertex3fv;
static pfn_glVertex3fv ptr_glDummyTestProc;

static GLboolean enableStaticTest = GL_FALSE;
static GLboolean enableGeneratedTest = GL_FALSE;
static GLboolean enablePatching = GL_FALSE;

int main(int argc, char **argv)
{
    int i;

    while (1) {
        int opt = getopt(argc, argv, "sgp");
        if (opt == -1) {
            break;
        }
        switch (opt) {
        case 's':
            enableStaticTest = GL_TRUE;
            break;
        case 'g':
            enableGeneratedTest = GL_TRUE;
            break;
        case 'p':
            enablePatching = GL_TRUE;
            break;
        default:
            return 1;
        }
    };

    __glDispatchInit();
    InitDummyVendors();

    ptr_glVertex3fv = (pfn_glVertex3fv) __glDispatchGetProcAddress("glVertex3fv");
    if (ptr_glVertex3fv == NULL) {
        printf("Can't find dispatch function for glVertex3fv\n");
    }

    if (enableGeneratedTest) {
        ptr_glDummyTestProc = (pfn_glVertex3fv) __glDispatchGetProcAddress(GENERATED_FUNCTION_NAME);
        if (ptr_glDummyTestProc == NULL) {
            printf("Can't find dispatch function for %s\n", GENERATED_FUNCTION_NAME);
        }
    }

    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        if (!TestDispatch(i, enableStaticTest, enableGeneratedTest)) {
            return 1;
        }
    }

    CleanupDummyVendors();
    __glDispatchFini();
    return 0;
}

static void InitDummyVendors(void)
{
    int i;
    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        dummyVendors[i].vendorID = __glDispatchNewVendorID();
        if (dummyVendors[i].vendorID == 0) {
            printf("__glDispatchNewVendorID failed\n");
            abort();
        }

        dummyVendors[i].dispatch = __glDispatchCreateTable(
                dummyVendors[i].getProcCallback, &dummyVendors[i]);
        if (dummyVendors[i].dispatch == NULL) {
            printf("__glDispatchCreateTable failed\n");
            abort();
        }
    }

    if (enablePatching) {
        dummyVendors[0].patchCallbacks.isPatchSupported = dummyCheckPatchSupported;
        dummyVendors[0].patchCallbacks.initiatePatch = dummy0_InitiatePatch;
        dummyVendors[0].patchCallbacksPtr = &dummyVendors[0].patchCallbacks;

        dummyVendors[1].patchCallbacks.isPatchSupported = dummyCheckPatchSupported;
        dummyVendors[1].patchCallbacks.initiatePatch = dummy1_InitiatePatch;
        dummyVendors[1].patchCallbacksPtr = &dummyVendors[1].patchCallbacks;
    }
}

static void CleanupDummyVendors(void)
{
    int i;
    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        if (dummyVendors[i].dispatch != NULL) {
            __glDispatchDestroyTable(dummyVendors[i].dispatch);
            dummyVendors[i].dispatch = NULL;
        }
    }
}

static void ResetCallCounts(void)
{
    int i, j;
    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        for (j=0; j<CALL_INDEX_COUNT; j++) {
            dummyVendors[i].callCounts[j] = 0;
        }
    }
}

static GLboolean CheckCallCounts(int expectedVendorIndex, int expectedCallIndex, int count)
{
    int vendorIndex, callIndex;
    GLboolean result = GL_TRUE;

    for (vendorIndex=0; vendorIndex<DUMMY_VENDOR_COUNT; vendorIndex++) {
        for (callIndex=0; callIndex<CALL_INDEX_COUNT; callIndex++) {
            int expected;
            if (vendorIndex == expectedVendorIndex && callIndex == expectedCallIndex) {
                expected = count;
            } else {
                expected = 0;
            }

            if (dummyVendors[vendorIndex].callCounts[callIndex] != expected) {
                printf("Wrong value for vendor %d, call %d: Expected %d, got %d\n",
                        vendorIndex, callIndex, expected,
                        dummyVendors[vendorIndex].callCounts[callIndex]);
                result = GL_FALSE;
            }
        }
    }
    return result;
}

static GLboolean TestDispatch(int vendorIndex,
        GLboolean testStatic, GLboolean testGenerated)
{
    int i;
    GLboolean result = GL_FALSE;
    GLboolean patched = (dummyVendors[vendorIndex].patchCallbacksPtr != NULL);

    if (!__glDispatchMakeCurrent(&dummyVendors[vendorIndex].threadState,
                dummyVendors[vendorIndex].dispatch, dummyVendors[vendorIndex].vendorID,
                dummyVendors[vendorIndex].patchCallbacksPtr)) {
        printf("__glDispatchMakeCurrent failed\n");
        return GL_FALSE;
    }

    printf("Testing vendor %d, patched = %d\n", vendorIndex, (int) patched);
    if (testStatic) {
        int callIndex = (patched ? CALL_INDEX_STATIC_PATCH : CALL_INDEX_STATIC);

        printf("Testing static dispatch through libOpenGL\n");
        ResetCallCounts();
        for (i = 0; i < NUM_GLDISPATCH_CALLS; i++) {
            glVertex3fv(NULL);
        }
        if (!CheckCallCounts(vendorIndex, callIndex, NUM_GLDISPATCH_CALLS)) {
            goto done;
        }

        printf("Testing static dispatch through GetProcAddress\n");
        ResetCallCounts();
        for (i = 0; i < NUM_GLDISPATCH_CALLS; i++) {
            ptr_glVertex3fv(NULL);
        }
        if (!CheckCallCounts(vendorIndex, callIndex, NUM_GLDISPATCH_CALLS)) {
            goto done;
        }
    }

    if (testGenerated) {
        int callIndex = (patched ? CALL_INDEX_GENERATED_PATCH : CALL_INDEX_GENERATED);

        printf("Testing generated dispatch\n");
        ResetCallCounts();
        for (i = 0; i < NUM_GLDISPATCH_CALLS; i++) {
            ptr_glDummyTestProc(NULL);
        }
        if (!CheckCallCounts(vendorIndex, callIndex, NUM_GLDISPATCH_CALLS)) {
            goto done;
        }
    }

    result = GL_TRUE;

done:
    __glDispatchLoseCurrent();
    return result;
}

static void *common_getProcAddressCallback(const char *procName, void *param, int vendorIndex)
{
    DummyVendorLib *dummyVendor = (DummyVendorLib *) param;
    if (dummyVendor != &dummyVendors[vendorIndex]) {
        printf("getProcAddress for vendor %d called with the wrong parameter\n", vendorIndex);
        abort();
    }

    if (strcmp(procName, "glVertex3fv") == 0) {
        return dummyVendor->vertexProc;
    } else if (strcmp(procName, GENERATED_FUNCTION_NAME) == 0) {
        return dummyVendor->testProc;
    } else {
        return NULL;
    }
}

static void *dummy0_getProcAddressCallback(const char *procName, void *param)
{
    return common_getProcAddressCallback(procName, param, 0);
}

static void *dummy1_getProcAddressCallback(const char *procName, void *param)
{
    return common_getProcAddressCallback(procName, param, 1);
}

static void *dummy2_getProcAddressCallback(const char *procName, void *param)
{
    return common_getProcAddressCallback(procName, param, 2);
}

static void dummy0_glVertex3fv(const GLfloat *v)
{
    dummyVendors[0].callCounts[CALL_INDEX_STATIC]++;
}

static void dummy1_glVertex3fv(const GLfloat *v)
{
    dummyVendors[1].callCounts[CALL_INDEX_STATIC]++;
}

static void dummy2_glVertex3fv(const GLfloat *v)
{
    dummyVendors[2].callCounts[CALL_INDEX_STATIC]++;
}

static void dummy0_glDummyTestProc(const GLfloat *v)
{
    dummyVendors[0].callCounts[CALL_INDEX_GENERATED]++;
}

static void dummy1_glDummyTestProc(const GLfloat *v)
{
    dummyVendors[1].callCounts[CALL_INDEX_GENERATED]++;
}

static void dummy2_glDummyTestProc(const GLfloat *v)
{
    dummyVendors[2].callCounts[CALL_INDEX_GENERATED]++;
}

static GLboolean common_InitiatePatch(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset, int vendorIndex)
{
    if (!dummyPatchFunction(type, stubSize, lookupStubOffset, "Vertex3fv",
                &dummyVendors[vendorIndex].callCounts[CALL_INDEX_STATIC_PATCH])) {
        return GL_FALSE;
    }

    if (enableGeneratedTest) {
        if (!dummyPatchFunction(type, stubSize, lookupStubOffset, GENERATED_FUNCTION_NAME,
                    &dummyVendors[vendorIndex].callCounts[CALL_INDEX_GENERATED_PATCH])) {
            return GL_FALSE;
        }
    }
    return GL_TRUE;
}

static GLboolean dummy0_InitiatePatch(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset)
{
    return common_InitiatePatch(type, stubSize, lookupStubOffset, 0);
}

static GLboolean dummy1_InitiatePatch(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset)
{
    return common_InitiatePatch(type, stubSize, lookupStubOffset, 1);
}

