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

libegl_dispatch_stubs = static_library(
  'egl_dispatch_stubs',
  ['egldispatchstubs.c', g_egldispatchstubs_c, g_egldispatchstubs_h],
  include_directories : inc_include,
  gnu_symbol_visibility : 'hidden',
)

libEGL = shared_library(
  'EGL',
  [
    'libegl.c',
    'libeglcurrent.c',
    'libeglmapping.c',
    'libeglvendor.c',
    'libeglerror.c',
  ],
  c_args : [
    '-DDEFAULT_EGL_VENDOR_CONFIG_DIRS="@0@/glvnd/egl_vendor.d:@1@/glvnd/egl_vendor.d"'.format(
      join_paths(get_option('prefix'), get_option('sysconfdir')),
      join_paths(get_option('prefix'), get_option('datadir'))),
  ],
  include_directories : inc_include,
  link_args : '-Wl,-Bsymbolic',
  link_with : libegl_dispatch_stubs,
  dependencies : [
    dep_threads, dep_dl, dep_m, dep_x11_headers, idep_trace, idep_glvnd_pthread,
    idep_utils_misc, idep_cjson, idep_winsys_dispatch, idep_gldispatch,
  ],
  version : '1.1.0',
  install : true,
  gnu_symbol_visibility : 'hidden',
)

pkg.generate(
  libEGL,
  filebase : 'egl',
  description : 'EGL library and headers',
  version : '1.5',
)

test(
  'EGL symbols check',
  prog_py,
  args : [
    files_symbols_check,
    '--nm', prog_nm.path(),
    '--lib', libEGL,
    '--symbols-file', files('egl.symbols'),
  ],
  suite : ['egl', 'symbols'],
)
