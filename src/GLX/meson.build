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

inc_glx = include_directories('.')

libGLX = shared_library(
  'GLX',
  files('libglx.c', 'libglxmapping.c', 'libglxproto.c'),
  include_directories : [inc_include, inc_util, inc_uthash, inc_gldispatch],
  link_args : ['-Wl,-Bsymbolic'],
  link_with : [
    libgldispatch, libtrace, libglvnd_pthread, libglvnd_genentry,
    libutils_misc, libapp_error_check, libwinsys_dispatch,
  ],
  dependencies : [dep_dl, dep_threads, dep_glproto, dep_x11, dep_xext],
  version : '0',
  gnu_symbol_visibility : 'hidden',
  install : true,
)
