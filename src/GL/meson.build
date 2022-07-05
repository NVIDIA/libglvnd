
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

# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

libGL = shared_library(
  'GL',
  ['libgl.c', g_libglglxwrapper_c],
  include_directories : [inc_include],
  link_args : '-Wl,-Bsymbolic',
  dependencies : [
    dep_dl, idep_gldispatch, idep_glapi_gl, dep_glx, idep_utils_misc, dep_x11_headers
  ],
  version : '1.7.0',
  gnu_symbol_visibility : 'hidden',
  install : true,
)

pkg.generate(
  libGL,
  filebase : 'gl',
  description : 'Legacy OpenGL and GLX library and headers.',
  version : '1.2',
)

dep_gl = declare_dependency(
  link_with : libGL,
  include_directories : inc_include,
)

if meson.version().version_compare('>= 0.54')
  meson.override_dependency('gl', dep_gl)
endif

test(
  'GL symbols check',
  prog_py,
  args : [
    files_symbols_check,
    '--nm', prog_nm.path(),
    '--lib', libGL,
    '--symbols-file', files('gl.symbols'),
  ],
  suite : ['symbols'],
)
