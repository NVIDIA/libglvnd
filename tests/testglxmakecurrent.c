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
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#if defined(USE_PTHREADS)
#include <pthread.h>
#endif // defined(USE_PTHREADS)

#include "utils_misc.h"
#include "test_utils.h"

// For glMakeCurrentTestResults()
#include "dummy/GLX_dummy.h"

typedef struct TestOptionsRec {
    int iterations;
    int threads;
} TestOptions;

static void print_help(void)
{
    const char *help_string =
        "Options: \n"
        " -h, --help              Print this help message.\n"
        " -i<N>, --iterations=<N> Run N make current iterations in each thread \n"
        " -t<N>, --threads=<N>    Run with N threads.\n";
    printf("%s", help_string);
}

void init_options(int argc, char **argv, TestOptions *t)
{
    int c;

    static struct option long_options[] = {
        { "help", no_argument, NULL, 'h'},
        { "iterations", required_argument, NULL, 'i'},
        { "threads", required_argument, NULL, 't'},
        { NULL, no_argument, NULL, 0 }
    };

    // Initialize defaults
    t->iterations = 1;
    t->threads = 1;

    do {
        c = getopt_long(argc, argv, "hi:t:", long_options, NULL);
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

void *MakeCurrentThread(void *arg)
{
    struct window_info wi;
    PFNGLXMAKECURRENTTESTRESULTSPROC pMakeCurrentTestResults = NULL;
    GLXContext ctx = NULL;
    const GLfloat v[] = { 0, 0, 0 };
    GLboolean saw;
    GLContextCounts contextCounts = {};
    GLint BeginCount = 0;
    GLint EndCount = 0;
    GLint Vertex3fvCount = 0;
    int i;
    intptr_t ret = GL_FALSE;
    const TestOptions *t = (const TestOptions *)arg;
    Display *dpy;
    GLboolean success;

    memset(&wi, 0, sizeof(wi));

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        printError("No display! Please re-test with a running X server\n"
                   "and the DISPLAY environment variable set appropriately.\n");
        goto fail;
    }

    // Make sure that libGLX has loaded the vendor library.
    glXGetClientString(dpy, GLX_EXTENSIONS);

    // Test the robustness of GetProcAddress() by calling this separately for
    // each thread.
    pMakeCurrentTestResults = (PFNGLXMAKECURRENTTESTRESULTSPROC)
            glXGetProcAddress((const GLubyte *) "glXMakeCurrentTestResults");
    if (pMakeCurrentTestResults == NULL) {
        printError("Failed to get glXMakeCurrentTestResults() function!\n");
        goto fail;
    }

    success = testUtilsCreateWindow(dpy, &wi, 0);
    if (!success) {
        printError("Failed to create window!\n");
        goto fail;
    }

    ctx = glXCreateContext(dpy, wi.visinfo, NULL, True);
    if (!ctx) {
        printError("Failed to create a context!\n");
        goto fail;
    }

    for (i = 0; i < t->iterations; i++) {

        if (!glXMakeContextCurrent(dpy, wi.draw, wi.draw, ctx)) {
            printError("Failed to make current!\n");
            goto fail;
        }

        glBegin(GL_TRIANGLES); BeginCount++;
        glVertex3fv(v); Vertex3fvCount++;
        glVertex3fv(v); Vertex3fvCount++;
        glVertex3fv(v); Vertex3fvCount++;
        glEnd(); EndCount++;

        // Make a call to glXMakeCurrentTestResults() to get the function contextCounts.
        saw = GL_FALSE;
        memset(&contextCounts, 0, sizeof(contextCounts));
        pMakeCurrentTestResults(&saw, &contextCounts);

        if (!saw) {
            printError("Failed to dispatch glXMakeCurrentTestResults()!\n");
            goto fail;
        }

        // Verify we have the right function contextCounts
        if ((contextCounts.beginCount != BeginCount) ||
            (contextCounts.vertex3fvCount != Vertex3fvCount) ||
            (contextCounts.endCount != EndCount)) {
            printError("Mismatch of reported function call contextCounts "
                       "between the application and vendor library!\n");
            goto fail;
        }

        if (!glXMakeContextCurrent(dpy, None, None, NULL)) {
            printError("Failed to lose current!\n");
            goto fail;
        }

        // Try calling functions here. These should dispatch to NOP stubs
        // (hence the call to glVertex3fv shouldn't crash).
        glBegin(GL_TRIANGLES);
        glVertex3fv(NULL);
        glEnd();

        // Similarly the call to the dynamic function glXMakeCurrentTestResults()
        // should be a no-op.
        saw = GL_FALSE;
        pMakeCurrentTestResults(&saw, &contextCounts);
        if (saw) {
            printError("Dynamic function glXMakeCurrentTestResults() dispatched "
                       "to vendor library even though no context was current!\n");
            goto fail;
        }

    }

    // Success!
    ret = GL_TRUE;

fail:
    if (dpy != NULL) {
        if (ctx) {
            glXDestroyContext(dpy, ctx);
        }
        testUtilsDestroyWindow(dpy, &wi);
        XCloseDisplay(dpy);
    }

    return (void *)ret;
}

int main(int argc, char **argv)
{
    /*
     * Try creating a context, making current to it, and calling GL functions
     * while the context is current.
     */
    TestOptions t;
    void *ret;
    int all_ret = 0;

    init_options(argc, argv, &t);

    if (t.threads > 1) {
    }

    if (t.threads == 1) {
        ret = MakeCurrentThread((void *)&t);
        if (!ret) {
            all_ret = 1;
        }
    } else {
#if defined(USE_PTHREADS)
        pthread_t *threads = malloc(t.threads * sizeof(pthread_t));
        int i;

        XInitThreads();

        for (i = 0; i < t.threads; i++) {
            if (pthread_create(&threads[i], NULL, MakeCurrentThread, (void *)&t)
                != 0) {
                printError("Error in pthread_create(): %s\n", strerror(errno));
                exit(1);
            }
        }

        for (i = 0; i < t.threads; i++) {
            if (pthread_join(threads[i], &ret) != 0) {
                printError("Error in pthread_join(): %s\n", strerror(errno));
                exit(1);
            }
            if (!ret) {
                all_ret = 1;
            }
        }
        free(threads);
#else // defined(USE_PTHREADS)
        // This shouldn't happen. If it does, then something is messed up in
        // the test script.
        printError("Using threads with non-thread test\n");
        exit(1);
#endif // defined(USE_PTHREADS)
    }
    return all_ret;
}
