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

libpatchentrypoints = static_library(
  'patchentrypoints',
  ['patchentrypoints.c'],
  include_directories : [inc_include, inc_util, inc_dispatch],
)

prog_cp = find_program('cp')

if with_glx
  libGLX_dummy = shared_library(
    'GLX_dummy',
    ['GLX_dummy.c'],
    include_directories : [inc_dispatch, inc_glx, inc_include],
    link_with: [libpatchentrypoints],
    dependencies : [idep_trace, idep_utils_misc, dep_x11],
  )

  # The ICD loader expects to load a .so.0, but meson generates the .X.Y.Z
  # links at install time, the build time directories only have the plain .so
  # files, so we copy the .so to .so.0 so tests will work
  custom_target(
    'GLX_dummy.so.0',
    input : [libGLX_dummy],
    output : 'libGLX_dummy.so.0',
    command : [prog_cp, '@INPUT@', '@OUTPUT@'],
    build_by_default : true,
  )
endif

if get_option('egl')
  foreach v : ['0', '1']
    _lib = shared_library(
      'EGL_dummy' + v,
      ['EGL_dummy.c'],
      c_args : ['-DDUMMY_VENDOR_NAME="dummy@0@"'.format(v)],
      include_directories : [inc_include],
      link_with: [libpatchentrypoints],
      dependencies : [idep_glvnd_pthread],
    )

    set_variable('libEGL_dummy' + v, _lib)

    # see The comment in the with_glx block
    custom_target(
      'EGL_dummy@0@.so.0'.format(v),
      input : _lib,
      output : 'libEGL_dummy@0@.so.0'.format(v),
      command : [prog_cp, '@INPUT@', '@OUTPUT@'],
      build_by_default : true,
  )
  endforeach
endif

dummy_build_dir = meson.current_build_dir()

