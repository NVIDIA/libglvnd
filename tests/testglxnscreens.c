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
 * n-screens Test
 *
 * This test creates a window and context on each screen on the running X
 * server. For each screen, it makes current to the window on that screen and
 * calls some OpenGL entrypoints.
 */

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <dlfcn.h>
#include "x11glvnd.h"

#include "trace.h"
#include "glvnd_pthread.h"
#include "test_utils.h"

#include "GLX_dummy/GLX_dummy.h"

#define FAILIF(cond, ...) do {      \
    if (cond) {                     \
        printError(__VA_ARGS__);    \
        ret = 1;                    \
        goto cleanup;               \
    }                               \
} while (0)

typedef struct MakeCurrentScreenThreadArgsRec {
    int iterations;
    int firstScreen;
    // TODO: pthread_barrier_t *nextScreenBarrier;
    int numScreens;
    struct window_info *wi;
    GLXContext *ctxs;
    char **vendorNames;
} MakeCurrentScreenThreadArgs;

typedef struct TestOptionsRec {
    int iterations;
    int threads;
} TestOptions;

static void print_help(void)
{
    const char *help_string =
        "Options: \n"
        " -h, --help                Print this help message.\n"
        " -i<N>, --iterations=<N>   Run N make current iterations in each thread.\n"
        " -t<N>, --threads=<N>      Run with N threads.\n";

    printf("%s", help_string);
}

static void init_options(int argc, char **argv, TestOptions *t)
{
    int c;

    static struct option long_options[] = {
        { "help", no_argument, NULL, 'h'},
        { "iterations", required_argument, NULL, 'i'},
        { "threads", required_argument, NULL, 't'},
    };

    // Initialize defaults
    t->iterations = 1;
    t->threads = 1;

    do {
        c = getopt_long(argc, argv, "hi:t:l", long_options, NULL);
        switch (c) {
        case -1:
        default:
            break;
        case 'h':
            print_help();
            exit(0);
            break;
        case 'i':
            t->iterations = atoi(optarg);
            if (t->iterations < 1) {
                printError("1 or more iterations required!\n");
                print_help();
                exit(1);
            }
            break;
        case 't':
            t->threads = atoi(optarg);
            if (t->threads < 1) {
                printError("1 or more threads required!\n");
                print_help();
                exit(1);
            }
            break;
        }
    } while (c != -1);
}

void *MakeCurrentScreenThread(void *arg)
{
    MakeCurrentScreenThreadArgs *t;
    int i;
    int ret = 0;
    GLXContext *ctxs;
    struct window_info *wi;
    int screen, offset;
    int firstScreen, numScreens;
    char *vendor;

    t = (MakeCurrentScreenThreadArgs *)arg;
    wi = t->wi;
    ctxs = t->ctxs;
    numScreens = t->numScreens;
    firstScreen = t->firstScreen;

    for (i = 0; i < t->iterations; i++) {
        for (offset = 0; offset < numScreens; offset++) {
            screen = (firstScreen + offset) % numScreens;
            FAILIF(!glXMakeContextCurrent(wi[screen].dpy,
                                          wi[screen].win,
                                          wi[screen].win,
                                          ctxs[screen]),
                   "Failed to make current!\n");

            // TODO: Make a call to glMakeCurrentTestResults() to get the vendor
            // string.

            vendor = strdup("VendorString");

            DBG_PRINTF(0, "Screen %d has vendor \"%s\"\n", screen, vendor);

            if (strcmp(vendor, t->vendorNames[screen])) {
                printError("Vendor string mismatch on screen %d: "
                           "expected \"%s\", got \"%s\"\n",
                           screen, t->vendorNames[screen], vendor);
                ret = 1;
            }

            free(vendor);


            if (!(i % 2)) {
                // Test losing current as well
                FAILIF(!glXMakeContextCurrent(wi[screen].dpy,
                                              None,
                                              None,
                                              NULL),
                       "Failed to lose current!\n");
            }
            // TODO pthread_barrier_wait(nextScreenBarrier);
        }
    }
cleanup:
    return (void *)((uintptr_t)ret);
}

GLVNDPthreadFuncs pImp;

int main(int argc, char **argv)
{
    Display *dpy;
    int numScreens;
    int screen, initScreen = 0;
    struct window_info *wi = NULL;
    int ret = 0;
    GLXContext *ctxs;
    char **vendorNames;
    MakeCurrentScreenThreadArgs *tArgs = NULL;
    int major, event, error;
    TestOptions t;
    int i;

    init_options(argc, argv, &t);

    dpy = XOpenDisplay(NULL);
    FAILIF(!dpy, "No display!\n");
    numScreens = ScreenCount(dpy);
    FAILIF(numScreens < 0, "Invalid screen count!\n");

    FAILIF(!XQueryExtension(dpy, XGLV_EXTENSION_NAME, &major, &event, &error),
           "No " XGLV_EXTENSION_NAME " extension!\n");

    wi = malloc(sizeof(struct window_info) * numScreens);
    ctxs = malloc(sizeof(GLXContext) * numScreens);
    vendorNames = malloc(sizeof(char *) * numScreens);
    FAILIF(!wi || !ctxs || !vendorNames, "Out of memory!\n");

    tArgs = malloc(sizeof(*tArgs) * t.threads);
    for (i = 0; i < t.threads; i++) {
        tArgs[i].iterations = t.iterations;
        tArgs[i].firstScreen = i % numScreens;
        tArgs[i].numScreens = numScreens;
        tArgs[i].wi = wi;
        tArgs[i].ctxs = ctxs;
        tArgs[i].vendorNames = vendorNames;
    }

    for (; initScreen < numScreens; initScreen++) {
        FAILIF(!testUtilsCreateWindow(dpy, &wi[initScreen], initScreen),
               "Failed to create window for screen %d!\n", initScreen);

        ctxs[initScreen] = glXCreateContext(dpy, wi[initScreen].visinfo,
                                            NULL, GL_TRUE);
        FAILIF(!ctxs[initScreen], "Failed to create a context!\n");

        vendorNames[initScreen] = XGLVQueryScreenVendorMapping(dpy, initScreen);
    }

    // TODO: getprocaddress glMakeCurrentTestResults() function

    if (t.threads == 1) {
        ret = (int)!!MakeCurrentScreenThread((void *)&tArgs[0]);
    } else {
        glvnd_thread_t *threads = malloc(t.threads * sizeof(glvnd_thread_t));
        void *one_ret;

        XInitThreads();

        if (!glvndSetupPthreads(RTLD_DEFAULT, &pImp)) {
            exit(1);
        }
        for (i = 0; i < t.threads; i++) {
            FAILIF(pImp.create(&threads[i], NULL, MakeCurrentScreenThread,
                   (void *)&tArgs[i]) != 0, "Error in pthread_create(): %s\n",
                   strerror(errno));
        }

        for (i = 0; i < t.threads; i++) {
            FAILIF(pImp.join(threads[i], &one_ret) != 0,
                   "Error in pthread_join(): %s\n", strerror(errno));

            if (one_ret) {
                ret = 1;
            }
        }

        free(threads);
    }

cleanup:
    free(tArgs);

    if (wi) {
        for (screen = 0; screen < initScreen; screen++) {
            testUtilsDestroyWindow(dpy, &wi[screen]);
        }
    }

    free(wi);

    if (dpy) {
        XCloseDisplay(dpy);
    }

    return ret;
}
