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

/*
 * Small test to check that the x11glvnd extension is working properly
 */
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "x11glvnd.h"

int main(int argc, char **argv)
{
    Display *dpy;
    char *vendor = NULL;
    int screen, queriedScreen;
    int major, event, error;
    const char *quote;
    XID xid;
    int numScreens;

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        printf("No display!\n");
        return 1;
    }

    if (!XQueryExtension(dpy, XGLV_EXTENSION_NAME, &major, &event, &error)) {
        fprintf(stderr,
                XGLV_EXTENSION_NAME " extension not present. Please re-test\n"
               "on an X server with this extension loaded.\n");
        goto fail;
    }

    numScreens = ScreenCount(dpy);
    printf("%d screens\n", numScreens);

    for (screen = 0; screen < numScreens; screen++) {
        xid = RootWindow(dpy, screen);
        queriedScreen = XGLVQueryXIDScreenMapping(dpy, xid);
        if (screen != queriedScreen) {
            // Mismatch!
            fprintf(stderr, "Screen mismatch!\n");
            goto fail;
        }
        vendor = XGLVQueryScreenVendorMapping(dpy, screen);

        quote = vendor ? "\"" : "";

        printf("XID %d -> (screen %d, vendor %s%s%s)\n", (int)xid, screen,
               quote, vendor ? vendor : "unknown", quote);
    }

    XCloseDisplay(dpy);
    return 0;

fail:
    XCloseDisplay(dpy);
    return 1;
}
