/*
 * Copyright (c) 2021, NVIDIA CORPORATION.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "dummy/EGL_dummy.h"

static EGLDisplay dpy = EGL_NO_DISPLAY;
static sem_t worker_ready_semaphore;

void init_context(void)
{
    EGLContext ctx = eglCreateContext(dpy, NULL, EGL_NO_CONTEXT, NULL);
    if (ctx == EGL_NO_CONTEXT) {
        printf("eglCreateContext failed\n");
        fflush(stdout);
        abort();
    }

    if (!eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx)) {
        printf("eglMakeCurrent failed\n");
        fflush(stdout);
        abort();
    }
}

void *worker_proc(void *param)
{
    init_context();

    return NULL;
}

void *worker_release_thread_proc(void *param)
{
    init_context();

    eglReleaseThread();

    return NULL;
}


void *worker_keep_proc(void *param)
{
    init_context();

    sem_post(&worker_ready_semaphore);

    while (1)
    {
        sleep(1);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    const struct option OPTIONS[] = {
        { "main", no_argument, NULL, 'm' },
        { "thread", no_argument, NULL, 't' },
        { "release-thread", no_argument, NULL, 'r' },
        { "thread-keep", no_argument, NULL, 'k' },
        { NULL }
    };

    int option_main = 0;
    int option_thread = 0;
    int option_release_thread = 0;
    int option_thread_keep = 0;

    EGLint major, minor;

    while (1)
    {
        int c = getopt_long(argc, argv, "hmtrk", OPTIONS, NULL);
        if (c == -1)
        {
            break;
        }
        switch (c)
        {
            case 'm':
                option_main = 1;
                break;
            case 't':
                option_thread = 1;
                break;
            case 'r':
                option_release_thread = 1;
                break;
            case 'k':
                option_thread_keep = 1;
                break;
            default:
                return 2;
        }
    }

    sem_init(&worker_ready_semaphore, 0, 0);

    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) {
        printf("eglGetDisplay failed\n");
        return 2;
    }
    if (!eglInitialize(dpy, &major, &minor)) {
        printf("eglInitialize failed\n");
        return 2;
    }

    if (option_main) {
        printf("Setting current context on main thread\n");
        init_context();
    }

    if (option_thread) {
        pthread_t thread;

        printf("Starting and terminating worker thread\n");
        if (pthread_create(&thread, NULL, worker_proc, dpy) != 0) {
            printf("pthread_create failed\n");
            return 2;
        }

        pthread_join(thread, NULL);
    }

    if (option_release_thread) {
        pthread_t thread;

        printf("Starting and terminating worker thread\n");
        if (pthread_create(&thread, NULL, worker_release_thread_proc, dpy) != 0) {
            printf("pthread_create failed\n");
            return 2;
        }

        pthread_join(thread, NULL);
    }


    if (option_thread_keep) {
        pthread_t thread;

        printf("Starting and keeping worker thread\n");
        if (pthread_create(&thread, NULL, worker_keep_proc, dpy) != 0) {
            printf("pthread_create failed\n");
            return 2;
        }

        sem_wait(&worker_ready_semaphore);
    }

    return 0;
}
