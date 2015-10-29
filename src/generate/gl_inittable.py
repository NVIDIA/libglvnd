#!/usr/bin/env python

# (C) Copyright IBM Corporation 2004, 2005
# (C) Copyright Apple Inc. 2011
# (C) Copyright 2015, NVIDIA CORPORATION.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Authors:
#    Kyle Brenneman <kbrenneman@nvidia.com>
#
# Based on code ogiginally by:
#    Jeremy Huddleston <jeremyhu@apple.com>
#    Ian Romanick <idr@us.ibm.com>
#    Brian Nguyen <brnguyen@nvidia.com>

import sys
import genCommon

def _main():
    print(r"""
/* DO NOT EDIT - This file generated automatically by gl_inittable.py script */

/* GLXEXT is the define used in the xserver when the GLX extension is being
 * built.  Hijack this to determine whether this file is being built for the
 * server or the client.
 */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#if (defined(GLXEXT) && defined(HAVE_BACKTRACE)) \
	|| (!defined(GLXEXT) && defined(DEBUG) && !defined(_WIN32_WCE) && !defined(__CYGWIN__) && !defined(__MINGW32__) && !defined(__OpenBSD__))
#define USE_BACKTRACE
#endif

#ifdef USE_BACKTRACE
#include <execinfo.h>
#endif

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

#include "main/glheader.h"

#include "glapi.h"
#include "glapitable.h"

#ifdef GLXEXT
#include "os.h"
#endif

static void
__glapi_inittable_NoOp(void) {
    const char *fstr = "Unknown";

    /* Silence potential GCC warning for some #ifdef paths.
     */
    (void) fstr;
#if defined(USE_BACKTRACE)
#if !defined(GLXEXT)
    if (getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
#endif
    {
        void *frames[2];

        if(backtrace(frames, 2) == 2) {
            Dl_info info;
            dladdr(frames[1], &info);
            if(info.dli_sname)
                fstr = info.dli_sname;
        }

#if !defined(GLXEXT)
        fprintf(stderr, "Call to unimplemented API: %s\n", fstr);
#endif
    }
#endif
#if defined(GLXEXT)
    LogMessage(X_ERROR, "GLX: Call to unimplemented API: %s\n", fstr);
#endif
}

static void
__glapi_inittable_set_remaining_noop(struct _glapi_table *disp,
                                     size_t entries) {
    void **dispatch = (void **) disp;
    int i;

    /* ISO C is annoying sometimes */
    union {_glapi_proc p; void *v;} p;
    p.p = __glapi_inittable_NoOp;

    for(i=0; i < entries; i++)
        if(dispatch[i] == NULL)
            dispatch[i] = p.v;
}
""".lstrip("\n"))

    # We split populating the table into a bunch of smaller functions instead
    # of one huge one. If we put everything in one function, the it takes
    # forever to compile.
    functionCount = 0
    entriesPerFunction = 100
    for (index, func) in enumerate(genCommon.getFunctions(sys.argv[1:])):
        if (index % entriesPerFunction == 0):
            if (index > 0):
                print("}\n")
            print(r"""
static void
_glapi_init_table_from_callback_%d(struct _glapi_table *table,
                                size_t entries,
                                void *(*get_proc_addr)(const char *name, void *param),
                                void *param)
{
""".lstrip("\n") % (functionCount,))
            functionCount += 1

        print(r"""
    if(!table->{f.basename}) {{
        void ** procp = (void **) &table->{f.basename};
        *procp = (*get_proc_addr)("{f.name}", param);
    }}
""".format(f=func))

    print(r"""
}
void
_glapi_init_table_from_callback(struct _glapi_table *table,
                                size_t entries,
                                void *(*get_proc_addr)(const char *name, void *param),
                                void *param)
{
""".lstrip("\n"))

    for i in xrange(functionCount):
        print("    _glapi_init_table_from_callback_%d(table, entries, get_proc_addr, param);" % (i,))

    print("")
    print("    __glapi_inittable_set_remaining_noop(table, entries);")
    print("}\n")

if (__name__ == "__main__"):
    _main()

