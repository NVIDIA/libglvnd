# Copyright © 2018 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

inc_util = include_directories('.')
inc_uthash = include_directories('uthash/src')

libglvnd_pthread = static_library(
  'glvnd_pthread',
  ['glvnd_pthread.c'],
  include_directories : inc_include,
  gnu_symbol_visibility : 'hidden',
)

foreach s : ['app_error_check', 'utils_misc', 'glvnd_genentry', 'trace', 'cJSON']
  set_variable(
    'lib' + s,
    static_library(
      s,
      [s + '.c'],
      include_directories : [inc_include],
      gnu_symbol_visibility : 'hidden',
    )
  )
endforeach

libwinsys_dispatch = static_library(
  'winsys_dispatch',
  ['winsys_dispatch.c'],
  include_directories : [inc_include, inc_uthash],
  gnu_symbol_visibility : 'hidden',
)
