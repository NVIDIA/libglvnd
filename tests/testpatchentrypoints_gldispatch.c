#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>

#include <GLdispatch.h>

#include "dummy/patchentrypoints.h"

#define DUMMY_VENDOR_COUNT 3

#define NUM_GL_CALLS 100
#define NUM_GLDISPATCH_CALLS 50

typedef struct DummyVendorLibRec {
    __GLdispatchThreadState threadState;
    __GLdispatchTable *dispatch;
    int vendorID;
    const __GLdispatchPatchCallbacks *patchCallbacksPtr;
    int sawVertex3fv;
    __GLdispatchPatchCallbacks patchCallbacks;
} DummyVendorLib;

typedef void (* pfn_glVertex3fv) (const GLfloat *v);

static void initDummyVendors(void);
static void cleanupDummyVendors(void);
static GLboolean testDummyVendor(int index);

static void *getProcAddressCallback(const char *procName, void *param);
static void dummy_glVertex3fv(const GLfloat *v);

static GLboolean dummyInitiatePatch0(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset);
static GLboolean dummyInitiatePatch1(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset);

static DummyVendorLib dummyVendors[DUMMY_VENDOR_COUNT] = {};
static pfn_glVertex3fv ptr_glVertex3fv;

int main(int argc, char **argv)
{
    initDummyVendors();

    ptr_glVertex3fv = (pfn_glVertex3fv) __glDispatchGetProcAddress("glVertex3fv");
    if (ptr_glVertex3fv == NULL) {
        printf("Can't look up function glVertex3fv\n");
        return 1;
    }

    // Start with a quick sanity test. Make sure the normal dispatch table
    // works before we try patching anything.
    if (!testDummyVendor(2)) {
        return 1;
    }

    // Start with a vendor that supports patching. Even after releasing the
    // current context, the entrypoints will remain patched.
    if (!testDummyVendor(0)) {
        return 1;
    }

    // Test the same vendor again.
    if (!testDummyVendor(0)) {
        return 1;
    }

    // Switch to another vendor that also supports patching. This should
    // unpatch everything, then patch again with the new callbacks.
    if (!testDummyVendor(1)) {
        return 1;
    }

    // Switch to a vendor that doesn't support patching. This should unpatch
    // the entrypoints so that it goes through the normal dispatch table.
    if (!testDummyVendor(2)) {
        return 1;
    }

    cleanupDummyVendors();
    return 0;
}

static GLboolean testDummyVendor(int index)
{
    int i;
    GLboolean result;

    printf("Testing dummy vendor %d\n", index);

    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        dummyVendors[i].sawVertex3fv = 0;
    }

    if (!__glDispatchMakeCurrent(&dummyVendors[index].threadState,
                dummyVendors[index].dispatch, dummyVendors[index].vendorID,
                dummyVendors[index].patchCallbacksPtr)) {
        printf("__glDispatchMakeCurrent failed\n");
        return GL_FALSE;
    }

    for (i = 0; i < NUM_GL_CALLS; i++) {
        glVertex3fv(NULL);
    }
    for (i = 0; i < NUM_GLDISPATCH_CALLS; i++) {
        ptr_glVertex3fv(NULL);
    }

    // Make sure that the right variable was incremented.
    result = GL_TRUE;
    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        int expected;
        if (i == index) {
            expected = NUM_GL_CALLS + NUM_GLDISPATCH_CALLS;
        } else {
            expected = 0;
        }

        if (dummyVendors[i].sawVertex3fv != expected) {
            printf("Wrong value for sawVertex3fv at index %d: Expected %d, got %d\n",
                    i, expected, dummyVendors[i].sawVertex3fv);
            result = GL_FALSE;
        }
    }

    __glDispatchLoseCurrent();
    return result;
}

void initDummyVendors(void)
{
    int i;
    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        dummyVendors[i].vendorID = __glDispatchNewVendorID();
        dummyVendors[i].dispatch = __glDispatchCreateTable(
                getProcAddressCallback, &dummyVendors[i]);
    }

    // Set up entrypoint patching for two of the vendors.
    dummyVendors[0].patchCallbacks.isPatchSupported = dummyCheckPatchSupported;
    dummyVendors[0].patchCallbacks.initiatePatch = dummyInitiatePatch0;
    dummyVendors[0].patchCallbacksPtr = &dummyVendors[0].patchCallbacks;

    dummyVendors[1].patchCallbacks.isPatchSupported = dummyCheckPatchSupported;
    dummyVendors[1].patchCallbacks.initiatePatch = dummyInitiatePatch1;
    dummyVendors[1].patchCallbacksPtr = &dummyVendors[1].patchCallbacks;
}

void cleanupDummyVendors(void)
{
    int i;
    for (i=0; i<DUMMY_VENDOR_COUNT; i++) {
        if (dummyVendors[i].dispatch != NULL) {
            __glDispatchDestroyTable(dummyVendors[i].dispatch);
            dummyVendors[i].dispatch = NULL;
        }
    }

}

static GLboolean dummyInitiatePatch0(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset)
{
    return commonInitiatePatch(type, stubSize, lookupStubOffset, &dummyVendors[0].sawVertex3fv);
}

static GLboolean dummyInitiatePatch1(int type, int stubSize,
        DispatchPatchLookupStubOffset lookupStubOffset)
{
    return commonInitiatePatch(type, stubSize, lookupStubOffset, &dummyVendors[1].sawVertex3fv);
}

static void dummy_glVertex3fv(const GLfloat *v)
{
    dummyVendors[2].sawVertex3fv++;
}

static void *getProcAddressCallback(const char *procName, void *param)
{
    if (strcmp(procName, "glVertex3fv") == 0) {
        return dummy_glVertex3fv;
    }
    return NULL;
}

