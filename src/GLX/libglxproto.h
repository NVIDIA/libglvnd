/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
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

#ifndef LIBGLXPROTO_H
#define LIBGLXPROTO_H

/*!
 * Functions to handle various GLX protocol requests.
 */

#include "libglxmapping.h"

#ifndef GLX_VENDOR_NAMES_EXT
#define GLX_VENDOR_NAMES_EXT 0x20F6
#endif

#define GLX_EXT_LIBGLVND_NAME "GLX_EXT_libglvnd"

/*!
 * Sends a glXQueryServerString request. If an error occurs, then it will
 * return \c NULL, but won't call the X error handler.
 *
 * \param dpyInfo The display connection.
 * \param screen The screen number.
 * \param name The name enum to request.
 * \return The string, or \c NULL on error. The caller must free the string
 * using \c free.
 */
char *__glXQueryServerString(__GLXdisplayInfo *dpyInfo, int screen, int name);

/*!
 * Looks up the screen number for a drawable.
 *
 * Note that if the drawable is valid, but server doesn't send a screen number,
 * then that means the server doesn't support the GLX_EXT_libglvnd extension.
 * In that case, this function will return zero, since we'll be using the same
 * screen for every drawable anyway.
 *
 * \param dpyInfo The display connection.
 * \param drawable The drawable to query.
 * \return The screen number for the drawable, or -1 on error.
 */
int __glXGetDrawableScreen(__GLXdisplayInfo *dpyInfo, GLXDrawable drawable);

#endif // LIBGLXPROTO_H
