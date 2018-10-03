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

libpatchentrypoints = static_library(
  'patchentrypoints',
  ['patchentrypoints.c'],
  include_directories : [inc_include, inc_util, inc_gldispatch],
  gnu_symbol_visibility : 'hidden',
)

if build_glx
  libGLX_dummy = shared_library(
    'GLX_dummy',
    ['GLX_dummy.c'],
    include_directories : [inc_include, inc_util, inc_glx, inc_gldispatch],
    link_with : [libpatchentrypoints, libutils_misc, libtrace],
    dependencies : [dep_x11],
    gnu_symbol_visibility : 'hidden',
    version : '0',
  )
endif

if build_egl
  foreach a : ['dummy0', 'dummy1']
    set_variable(
      'libEGL_' + a,
      shared_module(
        'EGL_' + a,
        ['EGL_dummy.c'],
        c_args : ['-DDUMMY_VENDOR_NAME="@0@"'.format(a)],
        include_directories : [inc_include, inc_util],
        link_with : [libglvnd_pthread],
        gnu_symbol_visibility : 'hidden',
      )
    )
  endforeach
endif
