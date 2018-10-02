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

inc_gldispatch = include_directories('.', 'vnd-glapi')

subdir('vnd-glapi')

if use_tls
  _linker_script = 'export_list_tls.ver'
else
  _linker_script = 'export_list_tld.ver'
endif
_linker_script = join_paths(meson.current_source_dir(), _linker_script)

libgldispatch = shared_library(
  'GLdispatch',
  ['GLdispatch.c'],
  include_directories : [
    inc_include, inc_util, include_directories('vnd-glapi'),
  ],
  link_args : ['-Xlinker', '--version-script=@0@'.format(_linker_script)],
  link_depends : [_linker_script],
  link_with : [
    libtrace, libglvnd_pthread, libapp_error_check, libutils_misc, libglapi,
  ],
  dependencies : dep_dl,
  gnu_symbol_visibility : 'hidden',
  version : '0',
  install : true,
)
