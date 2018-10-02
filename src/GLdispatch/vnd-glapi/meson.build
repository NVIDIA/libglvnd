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

files_gldispatch = []
if gldispatch == 'c'
  files_gldispatch += files('entry_pure_c.c')
else
  files_gldispatch += files('entry_common.c')
  if gldispatch == 'armv7'
    files_gldispatch += files('entry_armv7_tsd.c')
  else
    files_gldispatch += files('entry_simple_asm.c', 'entry_@0@_@1@.c'.format(gldispatch, tls_suffix.to_lower()))
  endif
endif

files_libglapi = files(
  'mapi_glapi.c',
  'stub.c',
  'table.c',
  'u_execmem.c',
)
if use_tls
  files_libglapi += files('u_current_tls.c')
else
  files_libglapi += files('u_current_tld.c')
endif

libglapi = static_library(
  'glapi',
  [files_libglapi, files_gldispatch, glapi_mapi_tmp_h],
  c_args : ['-DMAPI_ABI_HEADER="@0@"'.format(glapi_mapi_tmp_h.full_path())],
  include_directories : [inc_include, inc_util],
  gnu_symbol_visibility : 'hidden',
)

foreach e : ['gl', 'opengl', 'glesv1', 'glesv2']
  _gen = get_variable('g_glapi_mapi_@0@_tmp_h'.format(e))
  set_variable(
    'libglapi_@0@'.format(e),
    static_library(
      'glapi_@0@'.format(e),
      ['stub.c', files_gldispatch, _gen],
      c_args : [
        '-DSTATIC_DISPATCH_ONLY',
        '-DMAPI_ABI_HEADER="@0@"'.format(_gen.full_path()),
      ],
      include_directories : [inc_include, inc_util],
      gnu_symbol_visibility : 'hidden',
      build_by_default : false,
    )
  )
endforeach
