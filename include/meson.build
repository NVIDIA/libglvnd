# Copyright © 2019 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:

# The above copyright notice and this permission notice shall be included
# unaltered in all copies or substantial portions of the Materials.
# Any additions, deletions, or changes to the original source files
# must be clearly indicated in accompanying documentation.

# If only executable code is distributed, then the accompanying
# documentation must state that "this software is based in part on the
# work of the Khronos Group."

# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

inc_include = include_directories('.')

install_headers(
  'glvnd/GLdispatchABI.h',
  'glvnd/libglxabi.h',
  'glvnd/libeglabi.h',
  subdir : 'glvnd'
)

_headers = get_option('headers')

if _headers
  install_headers(
    'GL/gl.h',
    'GL/glcorearb.h',
    'GL/glext.h',
    subdir : 'GL',
  )

  install_headers(
    'KHR/khrplatform.h',
    subdir : 'KHR',
  )
endif

if with_glx and _headers
  install_headers(
    'GL/glx.h',
    'GL/glxext.h',
    subdir : 'GL',
  )
endif

if get_option('gles1') and _headers
  install_headers(
    'GLES/egl.h',
    'GLES/gl.h',
    'GLES/glext.h',
    'GLES/glplatform.h',
    subdir : 'GLES'
  )
endif

if get_option('gles2') and _headers
  install_headers(
    'GLES2/gl2ext.h',
    'GLES2/gl2.h',
    'GLES2/gl2platform.h',
    subdir : 'GLES2',
  )
  install_headers(
    'GLES3/gl31.h',
    'GLES3/gl32.h',
    'GLES3/gl3.h',
    'GLES3/gl3ext.h',
    'GLES3/gl3platform.h',
    subdir : 'GLES3',
  )
endif

if get_option('egl') and _headers
  install_headers(
    'EGL/egl.h',
    'EGL/eglext.h',
    'EGL/eglplatform.h',
    subdir : 'EGL',
  )
endif

