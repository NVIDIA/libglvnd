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

libegl_dispatch_stubs = static_library(
  'egl_dispatch_stubs',
  ['egldispatchstubs.c', g_egldispatchstubs_c, g_egldispatchstubs_h],
  include_directories : [inc_include],
  gnu_symbol_visibility : 'hidden',
)

_config_dir = '@0@:@1@'.format(
  join_paths(get_option('prefix'), get_option('sysconfdir'), 'glvnd', 'egl_vendor.d'),
  join_paths(get_option('prefix'), get_option('datadir'), 'glvnd', 'egl_vendor.d'),
)

libEGL = shared_library(
  'EGL',
  files(
    'libegl.c', 'libeglcurrent.c', 'libeglmapping.c', 'libeglvendor.c',
    'libeglerror.c',
  ),
  include_directories : [inc_uthash, inc_util, inc_gldispatch, inc_include],
  c_args : ['-DDEFAULT_EGL_VENDOR_CONFIG_DIRS="@0@"'.format(_config_dir)],
  link_args : ['-Wl,-Bsymbolic'],
  link_with : [
    libgldispatch, libtrace, libglvnd_pthread, libutils_misc, libcJSON,
    libwinsys_dispatch, libegl_dispatch_stubs,
  ],
  dependencies : [dep_m, dep_dl, dep_threads],
  version : '1.1.0',
  gnu_symbol_visibility : 'hidden',
  install : true,
)
