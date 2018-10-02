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

subdir('dummy')

# disable "unused" warnings
_test_c_args = ['-Wno-error=unused-variable', '-Wno-error=unused-label']

testgldispatch = executable(
  'testgldispatch',
  ['testgldispatch.c'],
  c_args : _test_c_args,
  dependencies : [dep_threads],
  include_directories : [inc_include, inc_gldispatch],
  link_with : [libgldispatch, libOpenGL, libpatchentrypoints, libutils_misc],
)

foreach t : [['static', ['-s']],
             ['static threaded', ['-s', '-t']],
             ['generated', ['-g']],
             ['generated threaded', ['-g', '-t']],
             ['patched', ['-s', '-g', '-p']],
             ['patched threaded', ['-s', '-g', '-p', '-t']]]
  test('gldispatch ' + t[0], testgldispatch, args : t[1])
endforeach

if build_glx
  glxenv = environment()
  glxenv.set('__GLX_VENDOR_LIBRARY_NAME', 'dummy')
  glxenv.set('LD_LIBRARY_PATH', join_paths(meson.current_build_dir(), 'dummy'))

  test(
    'GLXCreateContext',
    executable(
      'testglxcreatecontext',
      ['testglxcreatecontext.c', 'test_utils.c'],
      c_args : _test_c_args,
      link_with : [libGLX, libOpenGL],
      dependencies : [dep_x11],
    ),
    workdir : meson.current_build_dir(),
    env : glxenv,
  )

  testglxmakecurrent = executable(
    'testglxmakecurrent',
    ['testglxmakecurrent.c', 'test_utils.c'],
    include_directories : [inc_util],
    link_with : [libGLX, libOpenGL, libglvnd_pthread, libtrace],
    dependencies : [dep_x11, dep_dl],
  )

  foreach t : [['basic', ['-t', '1', '-i', '1']],
               ['late', ['-t', '1', '-i', '1', '-l']],
               ['loop', ['-t', '1', '-i', '250']],
               ['threads', ['-t', '5', '-i', '20000']]]
    test(
      'GLXMakeCurrent ' + t[0],
      testglxmakecurrent,
      args : t[1],
      env : glxenv,
    )
  endforeach

  test(
    'GLXMakeCurrent (old link)',
    executable(
      'testglxmakecurrent_oldlink',
      ['testglxmakecurrent.c', 'test_utils.c'],
      c_args : _test_c_args,
      include_directories : [inc_util],
      link_with : [libGL, libglvnd_pthread, libtrace],
      dependencies : [dep_x11, dep_dl],
    ),
    args : ['-t', '1', '-i', '1'],
    env : glxenv,
  )

  test(
    'GLXGetProcAddress',
    executable(
      'testglxgetprocaddress',
      ['testglxgetprocaddress.c'],
      c_args : _test_c_args,
      link_with : [libGLX],
      dependencies : [dep_x11],
    ),
    env : glxenv,
  )

  test(
    'GLXGetClientStr',
    executable(
      'testglxgetclientstr',
      ['testglxgetclientstr.c'],
      c_args : _test_c_args,
      include_directories : [inc_util],
      link_with : [libGLX, libOpenGL, libtrace],
      dependencies : [dep_x11],
    ),
    env : glxenv,
  )

  test(
    'GLXQueryVersion',
    executable(
      'testglxqueryversion',
      ['testglxqueryversion.c'],
      c_args : _test_c_args,
      link_with : [libGLX, libOpenGL],
      dependencies : [dep_x11],
    ),
    env : glxenv,
  )

  # TODO: Should extend meson to allow copying environments.
  glxenv2 = environment()
  glxenv2.set('__GLX_VENDOR_LIBRARY_NAME', 'dummy')
  glxenv2.set('LD_LIBRARY_PATH', join_paths(meson.current_build_dir(), 'dummy'))
  glxenv2.set('GLVND_TEST_PATCH_ENTRYPOINTS', '1')
  test(
    'glx patch entry points',
    executable(
      'testpatchentrypoints',
      ['testpatchentrypoints.c', 'test_utils.c'],
      c_args : _test_c_args,
      include_directories : [inc_util],
      link_with : [libGLX, libOpenGL, libtrace],
      dependencies : [dep_x11, dep_dl],
    ),
    env : glxenv2,
  )
endif

if build_egl
  eglenv = environment()
  eglenv.set('__EGL_VENDOR_LIBRARY_DIRS', join_paths(meson.current_source_dir(), 'json'))
  eglenv.set('LD_LIBRARY_PATH', join_paths(meson.current_build_dir(), 'dummy'))

  foreach t : [['testegldisplay', []],
               ['testegldevice', []],
               ['testeglgetprocaddress', []],
               ['testeglmakecurrent', [libOpenGL]],
               ['testeglerror', [libOpenGL]],
               ['testegldebug', []]]
    test(
      t[0],
      executable(
        t[0],
        [t[0] + '.c', 'egl_test_utils.c'],
        c_args : _test_c_args,
        include_directories : [inc_util],
        link_with : [libEGL, t[1]]
      ),
      env : eglenv,
    )
  endforeach
endif
