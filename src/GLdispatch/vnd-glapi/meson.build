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

inc_vnd_glapi = include_directories('.')

_libglapi_sources = []
if have_tls
  _libglapi_sources += 'u_current_tls.c'
else
  _libglapi_sources += 'u_current_tsd.c'
endif

_entry_files = []
if gl_dispatch_type == 'pure_c'
  _entry_files += 'entry_pure_c.c'
else
  _entry_files += 'entry_common.c'
  if gl_dispatch_type != 'armv7_tsd'
    _entry_files += 'entry_simple_asm.c'
  endif
  if gl_dispatch_type == 'x86_tls'
    _entry_files += 'entry_x86_tls.c'
  elif gl_dispatch_type == 'x86_tsd'
    _entry_files += 'entry_x86_tsd.c'
  elif gl_dispatch_type == 'x86_64_tls'
    _entry_files += 'entry_x86_64_tls.c'
  elif gl_dispatch_type == 'x86_64_tsd'
    _entry_files += 'entry_x86_64_tsd.c'
  elif gl_dispatch_type == 'armv7_tsd'
    _entry_files += 'entry_armv7_tsd.c'
  elif gl_dispatch_type == 'aarch64_tsd'
    _entry_files += 'entry_aarch64_tsd.c'
  elif gl_dispatch_type == 'ppc64_tls'
    _entry_files += 'entry_ppc64_tls.c'
  elif gl_dispatch_type == 'ppc64_tsd'
    _entry_files += 'entry_ppc64_tsd.c'
  else
    error('No matching ASM file for @0@'.format(gl_dispatch_type))
  endif
endif

libglapi = static_library(
  'libglapi',
  [
    'mapi_glapi.c',
    'stub.c',
    'table.c',
    _libglapi_sources,
    glapi_mapi_tmp_h,
    _entry_files,
  ],
  c_args : ['-DMAPI_ABI_HEADER="@0@"'.format(glapi_mapi_tmp_h.full_path())],
  include_directories : inc_include,
  dependencies : idep_utils_misc,
  gnu_symbol_visibility : 'hidden',
)

foreach g : ['gl', 'opengl', 'glesv1', 'glesv2']
  name = 'glapi_' + g
  header = get_variable('g_glapi_mapi_@0@_tmp_h'.format(g))

  _lib = static_library(
    name,
    ['stub.c', _entry_files, header],
    c_args : [
      '-DSTATIC_DISPATCH_ONLY',
      '-DMAPI_ABI_HEADER="@0@"'.format(header.full_path()),
    ],
    include_directories : [inc_include, inc_util],
    gnu_symbol_visibility : 'hidden',
  )

  _dep = declare_dependency(
    link_with : _lib,
    include_directories : inc_vnd_glapi,
  )

  set_variable('idep_' + name, _dep)
endforeach

