/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "entry.h"
#include "u_current.h"
#include "u_macros.h"

/* define macros for use by assembly dispatchers */
#define ENTRY_CURRENT_TABLE U_STRINGIFY(u_current)

#define ENTRY_CURRENT_TABLE_GET U_STRINGIFY(u_current_get_internal)

#if defined(USE_X86_ASM) && defined(__GNUC__)
#   ifdef GLX_USE_TLS
#      include "entry_x86_tls.h"
#   else                 
#      include "entry_x86_tsd.h"
#   endif
#elif defined(USE_X86_64_ASM) && defined(__GNUC__)
#   ifdef GLX_USE_TLS
#      include "entry_x86-64_tls.h"
#   else
#      include "entry_x86-64_tsd.h"
#   endif
#else
#   include "entry_pure_c.h"
#endif

