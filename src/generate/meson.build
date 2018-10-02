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

foreach m : [['gldispatch', 'glapi_mapi_tmp.h'],
             ['gl', 'g_glapi_mapi_gl_tmp.h'],
             ['opengl', 'g_glapi_mapi_opengl_tmp.h'],
             ['glesv1', 'g_glapi_mapi_glesv1_tmp.h'],
             ['glesv2', 'g_glapi_mapi_glesv2_tmp.h']]
  _mode = m[0]
  _name = m[1]

  set_variable(
    _name.underscorify(),
    custom_target(
      _name,
      input : ['gen_gldispatch_mapi.py', 'xml/gl.xml', 'xml/gl_other.xml'],
      output : _name,
      command : [prog_python, '@INPUT0@', _mode, '@INPUT1@', '@INPUT2@'],
      depend_files : files('genCommon.py'),
      capture : true,
    )
  )
endforeach

if build_egl
  foreach t : [['header', '.h'], ['source', '.c']]
    _name = 'g_egldispatchstubs' + t[1]
    _mode = t[0]

    set_variable(
      _name.underscorify(),
      custom_target(
        _name,
        input : ['gen_egl_dispatch.py', 'eglFunctionList.py', 'xml/egl.xml'],
        output : _name,
        command : [prog_python, '@INPUT0@', _mode, '@INPUT1@', '@INPUT2@'],
        depend_files : files('genCommon.py'),
        capture : true,
      )
    )
  endforeach
endif

if build_glx
  g_libglxwrapper_c = custom_target(
    'g_libglxwrapper.c',
    input : ['gen_libgl_glxstubs.py', 'xml/glx.xml', 'xml/glx_other.xml'],
    output : ['g_libglxwrapper.c'],
    command : [prog_python, '@INPUT@'],
    depend_files : files('genCommon.py'),
    capture : true,
  )
endif
