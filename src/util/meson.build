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

inc_util = include_directories('.')

libglvnd_pthread = static_library(
  'glvnd_pthread',
  ['glvnd_pthread.c'],
  dependencies : dep_dl,
  gnu_symbol_visibility : 'hidden',
)

idep_glvnd_pthread = declare_dependency(
  link_with : libglvnd_pthread,
  include_directories : inc_util,
)

libapp_error_check = static_library(
  'app_error_check',
  ['app_error_check.c'],
  include_directories : inc_include,
  gnu_symbol_visibility : 'hidden',
)

idep_app_error_check = declare_dependency(
  link_with : libapp_error_check,
  include_directories : inc_util,
)

libutils_misc = static_library(
  'utils_misc',
  ['utils_misc.c'],
  gnu_symbol_visibility : 'hidden',
)

idep_utils_misc = declare_dependency(
  link_with : libutils_misc,
  include_directories : inc_util,
)

libtrace = static_library(
  'trace',
  ['trace.c'],
  gnu_symbol_visibility : 'hidden',
)

idep_trace = declare_dependency(
  link_with : libtrace,
  include_directories : inc_util,
)

inc_uthash = include_directories('uthash/src')

libwinsys_dispatch = static_library(
  'winsys_dispatch',
  ['winsys_dispatch.c'],
  include_directories : [inc_include, inc_uthash],
  gnu_symbol_visibility : 'hidden',
)

idep_winsys_dispatch = declare_dependency(
  link_with : libwinsys_dispatch,
  include_directories : [inc_util, inc_uthash],
)

libcjson = static_library(
  'cJSON',
  ['cJSON.c'],
  gnu_symbol_visibility : 'hidden',
)

idep_cjson = declare_dependency(
  link_with : libcjson,
  include_directories : inc_util,
)

