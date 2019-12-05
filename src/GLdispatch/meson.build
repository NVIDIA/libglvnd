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

subdir('vnd-glapi')

if have_tls
  _ver_script = 'export_list_tls.ver'
else
  _ver_script = 'export_list_tsd.ver'
endif
_ver_script = join_paths(meson.current_source_dir(), _ver_script)

libgldispatch = shared_library(
  'GLdispatch',
  ['GLdispatch.c'],
  include_directories : [include_directories('vnd-glapi'), inc_include],
  link_args : ['-Wl,--version-script', _ver_script],
  link_with : libglapi,
  dependencies : [
    idep_trace, idep_glvnd_pthread, idep_app_error_check, dep_dl,
  ],
  gnu_symbol_visibility : 'hidden',
  link_depends : [_ver_script],
  install : true,
  version : '0.0.0',
)

inc_dispatch = include_directories('.')

idep_gldispatch = declare_dependency(
  link_with : libgldispatch,
  include_directories : inc_dispatch,
)

