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

subdir('dummy')

exe_gldispatch = executable(
  'testgldispatch',
  ['testgldispatch.c'],
  include_directories : [inc_include],
  link_with : [libOpenGL, libpatchentrypoints],
  dependencies : [idep_gldispatch, idep_utils_misc, dep_threads],
)

foreach k : [['static', ['-s']],
             ['static thr', ['-s', '-t']],
             ['generated', ['-g']],
             ['generated end', ['-g', '-l']],
             ['generated thr', ['-g', '-t']],
             ['generated thr end', ['-g', '-t', '-l']],
             ['patched', ['-s', '-g', '-p']],
             ['patched end', ['-s', '-g', '-p', '-l']],
             ['patched thr', ['-s', '-g', '-p', '-t']],
             ['patched thr end', ['-s', '-g', '-p', '-t', '-l']]]
  test(
    'gldispatch ' + k[0],
    exe_gldispatch,
    args : k[1],
    suite : ['gldispatch'],
  )
endforeach

test(
  'testgldispatchthread',
  executable(
    'testgldispatchthread',
    ['testgldispatchthread.c'],
    include_directories : [inc_include],
    dependencies : [idep_gldispatch, dep_threads],
  ),
  suite : ['gldispatch'],
)

_env_ld = 'LD_LIBRARY_PATH=@0@/'.format(dummy_build_dir)

if with_glx
  env_glx = [
    '__GLX_FORCE_VENDOR_LIBRARY_0=dummy',
    _env_ld,
  ]

  test(
    'glxcreatecontext',
    executable(
      'glxcreatecontext',
      ['testglxcreatecontext.c', 'test_utils.c'],
      include_directories : [inc_include],
      link_with : libOpenGL,
      dependencies : [idep_glx, dep_x11],
    ),
    env : env_glx,
    suite : ['glx'],
  )

  exe_glxmakecurrent = executable(
    'glxmakecurrent',
    ['testglxmakecurrent.c', 'test_utils.c'],
    include_directories : [inc_include],
    link_with : [libOpenGL],
    dependencies : [
      dep_dl, dep_x11, idep_glvnd_pthread, idep_trace, idep_utils_misc,
      idep_glx,
    ],
  )

  foreach t : [['basic', ['-t', '1', '-i', '1'], env_glx],
               ['loop', ['-t', '1', '-i', '250'], env_glx],
               ['threads', ['-t', '5', '-i', '20000'], [env_glx, 'LD_PRELOAD=libpthread.so.0']],
              ]
    test(
      'glxmakecurrent (@0@)'.format(t[0]),
      exe_glxmakecurrent,
      args : t[1],
      env : t[2],
      suite : ['glx'],
    )
  endforeach

  test(
    'glxmakecurrent (oldlink)',
    executable(
      'glxmakecurrent_oldlink',
      ['testglxmakecurrent.c', 'test_utils.c'],
      include_directories : [inc_include],
      link_with : [libGL],
      dependencies : [
        dep_dl, dep_x11, idep_glvnd_pthread, idep_trace, idep_utils_misc,
      ],
    ),
    args : ['-t', '1', '-i', '1'],
    env : env_glx,
    suite : ['glx'],
  )

  exe_glxgetprocaddress = executable(
    'glxgetprocaddress',
    ['testglxgetprocaddress.c'],
    include_directories : [inc_include],
    dependencies : [dep_x11, idep_glx, idep_gldispatch],
  )

  test(
    'glxgetprocess',
    exe_glxgetprocaddress,
    env : env_glx,
    suite : ['glx'],
  )

  test(
    'glxgetprocess_genentry',
    exe_glxgetprocaddress,
    env : env_glx,
    suite : ['glx'],
  )

  test(
    'glxgetprocaddress_genentry',
    executable(
      'testglxgetprocaddress_genentry',
      ['testglxgetprocaddress_genentry.c'],
      include_directories : [inc_include],
      dependencies : [dep_x11, idep_glx],
    ),
    env : env_glx,
    suite : ['glx'],
  )

  test(
    'glxgetclientstr',
    executable(
      'glxgetclientstr',
      ['testglxgetclientstr.c'],
      include_directories : [inc_include],
      link_with : libOpenGL,
      dependencies : [dep_x11, idep_glx, idep_trace, idep_utils_misc],
    ),
    env : env_glx,
    suite : ['glx'],
  )

  test(
    'glxgetqueryversion',
    executable(
      'glxqueryversion',
      ['testglxqueryversion.c'],
      include_directories : [inc_include],
      link_with : libOpenGL,
      dependencies : [dep_x11, idep_glx],
    ),
    env : env_glx,
    suite : ['glx'],
  )
endif

if get_option('egl')
  env_egl = [
    '__EGL_VENDOR_LIBRARY_DIRS=@0@'.format(join_paths(meson.current_source_dir(), 'json')),
    _env_ld,
  ]

  foreach t : [['egldisplay', [], []],
               ['egldevice', [], []],
               ['eglgetprocaddress', [], []],
               ['eglmakecurrent', [libOpenGL], [idep_utils_misc]],
               ['eglerror', [libOpenGL], []],
               ['egldebug', [], []]]
    test(
      t[0],
      executable(
        t[0],
        ['test@0@.c'.format(t[0]), 'egl_test_utils.c'],
        include_directories : [inc_include],
        link_with : [libEGL, t[1]],
        dependencies : [t[2]],
      ),
      env : env_egl,
      suite : ['egl'],
    )
  endforeach
endif

