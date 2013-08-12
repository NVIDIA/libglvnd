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

#include "utils_misc.h"
#include "test_utils.h"
#include "glvnd_pthread.h"

#include "GLX_dummy/GLX_dummy.h"

typedef struct TestOptionsRec {
    int iterations;
    int threads;
    GLboolean late;
} TestOptions;

static void print_help(void)
{
    const char *help_string =
        "Options: \n"
        " -h, --help              Print this help message.\n"
        " -i<N>, --iterations=<N> Run N make current iterations in each thread \n"
        " -t<N>, --threads=<N>    Run with N threads.\n"
        " -l, --late              Call GetProcAddress() after MakeCurrent()\n";
    printf("%s", help_string);
}

void init_options(int argc, char **argv, TestOptions *t)
{
    int c;

    static struct option long_options[] = {
        { "help", no_argument, NULL, 'h'},
        { "iterations", required_argument, NULL, 'i'},
        { "threads", required_argument, NULL, 't'},
        { "late", no_argument, NULL, 'l' },
        { NULL, no_argument, NULL, 0 }
    };

    // Initialize defaults
    t->iterations = 1;
    t->threads = 1;
    t->late = GL_FALSE;

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
        case 'l':
            t->late = GL_TRUE;
            break;
        }
    } while (c != -1);

}

void *MakeCurrentThread(void *arg)
{
    struct window_info wi;
    GLXContext ctx = NULL;
    const GLfloat v[] = { 0, 0, 0 };
    GLint BeginCount = 0;
    GLint EndCount = 0;
    GLint Vertex3fvCount = 0;
    int i;
    intptr_t ret = GL_FALSE;
    const TestOptions *t = (const TestOptions *)arg;
    Display *dpy;

    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        printError("No display! Please re-test with a running X server\n"
                   "and the DISPLAY environment variable set appropriately.\n");
        goto fail;
    }

    memset(&wi, 0, sizeof(wi));

    // Test the robustness of GetProcAddress() by calling this separately for
    // each thread.
    // TODO: retrieve the vendor library's test results function

    if (!testUtilsCreateWindow(dpy, &wi, 0)) {
        printError("Failed to create window!\n");
        goto fail;
    }

    // TODO: Might be a good idea to try sharing contexts/windows between
    // threads
    ctx = glXCreateContext(dpy, wi.visinfo, NULL, GL_TRUE);
    if (!ctx) {
        printError("Failed to create a context!\n");
        goto fail;
    }

    for (i = 0; i < t->iterations; i++) {

        if (!glXMakeContextCurrent(dpy, wi.win, wi.win, ctx)) {
            printError("Failed to make current!\n");
            goto fail;
        }

        if (t->late) {
            // TODO: retrieve the vendor library's test results function
        }

        glBegin(GL_TRIANGLES); BeginCount++;
        glVertex3fv(v); Vertex3fvCount++;
        glVertex3fv(v); Vertex3fvCount++;
        glVertex3fv(v); Vertex3fvCount++;
        glEnd(); EndCount++;

        // TODO: Make a call to glMakeCurrentTestResults() to get the function
        // counts. Compare them against the counts seen here.

        if (!glXMakeContextCurrent(dpy, None, None, NULL)) {
            printError("Failed to lose current!\n");
            goto fail;
        }

        // Try calling functions here. These should dispatch to NOP stubs
        // (hence the call to glVertex3fv shouldn't crash).
        glBegin(GL_TRIANGLES);
        glVertex3fv(NULL);
        glEnd();

        // TODO: Similarly the call to the dynamic function
        // glMakeCurrentTestResults() should be a no-op.

    }

    // Success!
    ret = GL_TRUE;

fail:
    if (ctx) {
        glXDestroyContext(dpy, ctx);
    }
    testUtilsDestroyWindow(dpy, &wi);

    return (void *)ret;
}

GLVNDPthreadFuncs pImp;

int main(int argc, char **argv)
{
    /*
     * Try creating a context, making current to it, and calling GL functions
     * while the context is current.
     */
    TestOptions t;
    int i;
    void *ret;

    init_options(argc, argv, &t);

    if (t.threads == 1) {
        ret = MakeCurrentThread((void *)&t);
        return ret ? 0 : 1;
    } else {
        glvnd_thread_t *threads = malloc(t.threads * sizeof(glvnd_thread_t));
        int all_ret = 0;

        XInitThreads();

        if (!glvndSetupPthreads(RTLD_DEFAULT, &pImp)) {
            exit(1);
        }

        for (i = 0; i < t.threads; i++) {
            if (pImp.create(&threads[i], NULL, MakeCurrentThread, (void *)&t)
                != 0) {
                printError("Error in pthread_create(): %s\n", strerror(errno));
                exit(1);
            }
        }

        for (i = 0; i < t.threads; i++) {
            if (pImp.join(threads[i], &ret) != 0) {
                printError("Error in pthread_join(): %s\n", strerror(errno));
                exit(1);
            }
            if (!ret) {
                all_ret = 1;
            }
        }
        return all_ret;
    }
}
