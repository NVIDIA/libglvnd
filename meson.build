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

project(
  'glvnd',
  ['c'],
  version : '1.1.0',
  meson_version : '>= 0.48.0',
  default_options : [
    'c_std=gnu99', 'buildtype=debugoptimized', 'b_ndebug=if-release',
  ]
)

add_project_arguments(['-D_GNU_SOURCE'], language : ['c'])

cc = meson.get_compiler('c')

# TODO: check the version
prog_python = import('python').find_installation('python2')

_asm = get_option('asm')
if not _asm.disabled()
  _host = host_machine.system()
  _arch = host_machine.cpu_family()
  _endian = host_machine.endian()

  # TODO: meson doesn't define a host for gnu*
  if _arch == 'x86' and ['freebsd', 'dragonfly', 'netbsd', 'linux'].contains(_host)
    add_project_arguments(
      ['-DUSE_X68_ASM', '-DUSE_MMX_ASM', '-DUSE_3DNOW_ASM', '-DUSE_SSE_ASM'],
      language : ['c']
    )
  elif _arch == 'x86_64' and ['freebsd', 'dragonfly', 'netbsd', 'linux'].contains(_host)
    add_project_arguments(['-DUSE_X68_64_ASM'], language : ['c'])
  elif _arch == 'arm' and _endian == 'little'
    add_project_arguments(['-DUSE_ARMV7_ASM'], language : ['c'])
  elif _arch == 'aarch64'
    add_project_arguments(['-DUSE_AARCH64_ASM'], language : ['c'])
  elif _arch == 'ppc64' and _endian == 'little'
    add_project_arguments(['-DUSE_PPC64LE_ASM'], language : ['c'])
  elif _asm.enabled()
    error('ASM required, but no Supported asm for OS: @0@ on arch: @1@'.format(_host, _arch))
  endif
endif

dep_threads = dependency('threads')
dep_x11 = dependency('x11')
dep_xext = dependency('xext')
dep_glproto = dependency('glproto')
dep_m = cc.find_library('m', required : false)

dep_dl = dependency('', required : false)
if not cc.has_function('dlopen')
  dep_dl = cc.find_library('dl')
endif

_tls = get_option('tls')
if not _tls.disabled()
  use_tls = cc.compiles(
      '__thread int foo __attribute__((tls_model("initial-exec")));',
      name : 'initial-exec TLS')
  if _tls.enabled() and not use_tls
    error('Initial-exec TLS required but unsupported')
  endif
else
  use_tls = false
endif

gldispatch = 'c'
if not _asm.disabled()
  if use_tls
    tls_suffix = 'TLS'
  else
    tls_suffix = 'TLD'
  endif
  if _arch.startswith('x86')
    add_project_arguments(
      ['-DGLDISPATCH_TYPE_@0@_@1@'.format(_arch.to_upper(), tls_suffix)],
      language : ['c'],
    )
    gldispatch = _arch
  elif _arch == 'ppc64' and _endian == 'little'
    add_project_arguments(
      ['-DGLDISPATCH_TYPE_PPC64LE_@0@'.format(tls_suffix)],
      language : ['c'],
    )
    gldispatch = 'ppc64le'
  elif _arch == 'arm' and _endian == 'little' and not _tls.enabled()
    add_project_arguments(['-DGLDISPATCH_TYPE_ARMV7_TSD'], language : ['c'])
    gldispatch = 'armv7'
    use_tls = false
  elif _arch == 'aarch64' and not _tls.enabled()
    add_project_arguments(['-DGLDISPATCH_TYPE_AARCH64_TSD'], language : ['c'])
    gldispatch = 'aarch64'
    use_tls = false
  endif
endif
if gldispatch == 'c'
  add_project_arguments(['-DGLDISPATCH_TYPE_PURE_C'], language : ['c'])
endif
if use_tls
  add_project_arguments(['-DGLDISPATCH_USE_TLS'], language : ['c'])
endif

if cc.has_function_attribute('constructor') and cc.has_function_attribute('destructor')
  add_project_arguments(['-DUSE_ATTRIBUTE_CONSTRUCTOR'], language : ['c'])
endif

if cc.compiles('''
    #include <pthread.h>
    void foo(void) {
      pthread_rwlock_t lock;
      pthread_rwlock_init(&lock, NULL);
    }''',
    args : '-D_GNU_SOURCE',
    name : 'pthread rwlocks')
  add_project_arguments(['-DHAVE_PTHREAD_RWLOCK_T'], language : ['c'])
endif

if cc.compiles('''
    int foo(int volatile *val, int oldVal, int newVal) {
      return __sync_add_and_fetch(val, 1);
      return __sync_lock_test_and_set(val, newVal);
      return __sync_val_compare_and_swap(val, oldVal, newVal);
    }''',
    name : 'sync intrinsics')
  add_project_arguments(['-DHAVE_SYNC_INTRINSICS'], language : ['c'])
endif

if cc.has_function('mincore')
  add_project_arguments(['-DHAVE_MINCORE'], language : ['c'])
endif

if cc.has_header_symbol('dlfcn.h', 'RTLD_NOLOAD')
  add_project_arguments(['-DHAVE_RTLD_NOLOAD'], language : ['c'])
endif

build_glx = get_option('glx')
build_egl = get_option('egl')
build_gles = get_option('gles')

subdir('include')
subdir('src')
subdir('tests')

pkg = import('pkgconfig')
pkg.generate(
  name : 'libglvnd',
  description : 'Vendor-neutral OpenGL dispatch library vendor interface',
  version : meson.project_version(),
)
