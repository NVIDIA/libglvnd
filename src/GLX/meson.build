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

g_glx_dispatch_stub_list_h = custom_target(
  'g_glx_dispatch_stub_list.h',
  input : 'gen_glx_stubs.py',
  output : 'g_glx_dispatch_stub_list.h',
  command : [prog_py, '@INPUT@'],
  capture : true,
)

libGLX = shared_library(
  'GLX',
  [
    'libglx.c',
    'libglxmapping.c',
    'libglxproto.c',
    'glvnd_genentry.c',
    g_glx_dispatch_stub_list_h,
  ],
  include_directories : [inc_include],
  link_args : '-Wl,-Bsymbolic',
  dependencies : [
    dep_dl, dep_x11, dep_glx, idep_gldispatch, idep_trace,
    idep_glvnd_pthread, idep_utils_misc,
    idep_app_error_check, idep_winsys_dispatch,
  ],
  gnu_symbol_visibility : 'hidden',
  install : true,
  version : '0.0.0',
)

inc_glx = include_directories('.')

idep_glx = declare_dependency(
  link_with : libGLX,
  include_directories : inc_glx,
)

pkg.generate(
  libGLX,
  filebase : 'glx',
  description : 'GLX library and headers.',
  version : '1.4',
)

test(
  'GLX symbols check',
  prog_py,
  args : [
    files_symbols_check,
    '--nm', prog_nm.path(),
    '--lib', libGLX,
    '--symbols-file', files('glx.symbols'),
  ],
  suite : ['glx', 'symbols'],
)
