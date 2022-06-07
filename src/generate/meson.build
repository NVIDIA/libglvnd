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

# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.


foreach t : [['glapi_mapi_tmp.h', 'gldispatch'],
             ['g_glapi_mapi_opengl_tmp.h', 'opengl'],
             ['g_glapi_mapi_glesv1_tmp.h', 'glesv1'],
             ['g_glapi_mapi_glesv2_tmp.h', 'glesv2']]
  file = t[0]
  target = t[1]
  var = file.underscorify()

  _t = custom_target(
    file,
    input : ['gen_gldispatch_mapi.py', 'xml/gl.xml', 'xml/gl_other.xml'],
    output : file,
    command : [prog_py, '@INPUT0@', target, '@INPUT1@', '@INPUT2@'],
    depend_files : files('genCommon.py'),
    capture : true,
  )

  set_variable(var, _t)
endforeach

# For libGL.so, we read the symbol list from a file instead of generating it
# from the XML files.
_t = custom_target(
  'g_glapi_mapi_gl_tmp.h',
  input : ['gen_gldispatch_mapi.py', '../GL/gl.symbols', 'xml/gl.xml', 'xml/gl_other.xml'],
  output : 'g_glapi_mapi_gl_tmp.h',
  command : [prog_py, '@INPUT0@', '@INPUT1@', '@INPUT2@', '@INPUT3@'],
  depend_files : files('genCommon.py'),
  capture : true,
)
set_variable('g_glapi_mapi_gl_tmp.h'.underscorify(), _t)

foreach target : ['header', 'source']
  ext = target == 'header' ? 'h' : 'c'
  _t = custom_target(
    'g_egldispatchstubs.' + ext,
    input : ['gen_egl_dispatch.py', 'xml/egl.xml'],
    output : ['g_egldispatchstubs.' + ext],
    command : [
      prog_py, '@INPUT0@', target, '@INPUT1@',
    ],
    depend_files : files('genCommon.py', 'eglFunctionList.py'),
    capture : true,
  )

  set_variable('g_egldispatchstubs_' + ext, _t)
endforeach

g_libglglxwrapper_c = custom_target(
  'g_libglglxwrapper.c',
  input : ['gen_libgl_glxstubs.py', 'xml/glx.xml', 'xml/glx_other.xml'],
  output : ['g_libglglxwrapper.c'],
  command : [prog_py, '@INPUT@'],
  depend_files : files('genCommon.py'),
  capture : true,
)

