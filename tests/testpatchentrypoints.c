#define _GNU_SOURCE 1

#include <string.h>
#include <X11/X.h>
#include <GL/gl.h>
#include <dlfcn.h>
#include "test_utils.h"

#define NUM_VERTEX3FV_CALLS 100

int main(int argc, char **argv)
{
    struct window_info wi;
    Display *dpy = XOpenDisplay(NULL);
    int sawVertex3fv1, *pSawVertex3fv;
    int i;
    int ret = 1;
    GLXContext ctx = None;
    void *vendorHandle;

    if (!dpy) {
        printError("No display!\n");
        goto fail;
    }

    memset(&wi, 0, sizeof(wi));

    if (!testUtilsCreateWindow(dpy, &wi, 0)) {
        printError("Failed to create window!\n");
        goto fail;
    }

    ctx = glXCreateContext(dpy, wi.visinfo, NULL, GL_TRUE);
    if (!ctx) {
        printError("Failed to create a context!\n");
        goto fail;
    }

    if (!glXMakeContextCurrent(dpy, wi.win, wi.win, ctx)) {
        printError("Failed to make current\n");
        goto fail;
    }

    vendorHandle = dlopen("libGLX_dummy.so", RTLD_LAZY);
    if (!vendorHandle) {
        printError("No valid vendor library handle\n");
        goto fail;
    }

    pSawVertex3fv = (int *)dlsym(vendorHandle, "__glXSawVertex3fv");
    if (!pSawVertex3fv) {
        printError("Could not find __glXSawVertex3fv\n");
        goto fail;
    }

    for (i = 0; i < NUM_VERTEX3FV_CALLS; i++) {
        glVertex3fv(NULL);
    }

    // Read the resulting value
    sawVertex3fv1 = *pSawVertex3fv;

    if (!glXMakeContextCurrent(dpy, None, None, NULL)) {
        printError("Could not lose current\n");
        goto fail;
    }

    dlclose(vendorHandle);
    pSawVertex3fv = NULL;

    if (sawVertex3fv1 != NUM_VERTEX3FV_CALLS) {
        printError("sawVertex3fv1 mismatch: expected %d, got %d\n",
                   NUM_VERTEX3FV_CALLS, sawVertex3fv1);
        goto fail;
    }

    ret = 0;

fail:
    if (ctx) {
        glXDestroyContext(dpy, ctx);
    }

    testUtilsDestroyWindow(dpy, &wi);

    return ret;
}
