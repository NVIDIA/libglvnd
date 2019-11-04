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

libopengl_main = static_library(
  'opengl_main',
  ['libopengl.c'],
  include_directories : [inc_util, inc_vnd_glapi, inc_include, inc_dispatch],
  gnu_symbol_visibility : 'hidden',
)

libOpenGL = shared_library(
  'OpenGL',
  link_whole : libopengl_main,
  dependencies : [idep_gldispatch, idep_glapi_opengl, idep_utils_misc],
  gnu_symbol_visibility : 'hidden',
  install : true,
  version : '0.0.0',
)

pkg.generate(
  libOpenGL,
  filebase : 'opengl',
  description : 'OpenGL (without GLX) library and headers.',
  version : '4.5',
)

test(
  'OpenGL symbols check',
  prog_py,
  args : [
    files_symbols_check,
    '--nm', prog_nm.path(),
    '--lib', libOpenGL,
    '--symbols-file', files('ogl.symbols'),
  ],
  suite : ['symbols'],
)
