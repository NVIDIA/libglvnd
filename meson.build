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

project(
  'glvnd',
  'c',
  version : '1.6.0',
  meson_version : '>= 0.48',
  default_options : ['c_std=gnu99']
)

if host_machine.system() in ['darwin', 'windows']
  error('Host OS: @0@ is unsupported.'.format(host_machine.system()))
endif

dep_null = dependency('', required : false)
cc = meson.get_compiler('c')
prog_py = import('python').find_installation()
files_symbols_check = files('bin/symbols-check.py')
prog_nm = find_program('nm')

message('Host CPU family: @0@'.format(host_machine.cpu_family()))
message('Host CPU: @0@'.format(host_machine.cpu()))

with_asm = get_option('asm')
use_asm = false
if not with_asm.disabled()
  use_asm = true
  if (host_machine.cpu_family() == 'x86' and
      ['gnu', 'freebsd', 'dragonfly', 'linux',
       'netbsd'].contains(host_machine.system()))
    add_project_arguments('-DUSE_X86_ASM', language : 'c')
  elif (host_machine.cpu_family() == 'x86_64' and
        ['freebsd', 'dragonfly', 'linux',
         'netbsd'].contains(host_machine.system()))
    add_project_arguments('-DUSE_X86_64_ASM', language : 'c')
  elif host_machine.cpu_family() == 'arm'
    # Try to figure out if we're targeting an ARMv7 or something older like
    # armel. Note that host_machine.cpu() won't help here -- it might still
    # return "armv7l" or "armv8l". Instead, try checking for compiler macros.
    if ['gcc', 'clang'].contains(cc.get_id())
      is_armv7 = cc.compiles('''
#if !(defined(__ARM_ARCH_7__) \
        || defined(__ARM_ARCH_7A__) \
        || defined(__ARM_ARCH_7R__) \
        || defined(__ARM_ARCH_7M__) \
        || defined(__ARM_ARCH_7S__) \
        || defined(__ARM_ARCH_6__) \
        || (defined(__ARM_ARCH) && __ARM_ARCH >= 6))
#error "Not ARM7"
#endif
      ''', name : 'ARMv7 macros')
    else
      is_armv7 = true
    endif
    if is_armv7
      add_project_arguments('-DUSE_ARMV7_ASM', language : 'c')
    else
      use_asm = false
    endif
  elif host_machine.cpu_family() == 'aarch64'
    add_project_arguments('-DUSE_AARCH64_ASM', language : 'c')
  elif host_machine.cpu_family() == 'ppc64' and cc.get_define('_CALL_ELF') == '2'
    add_project_arguments('-DUSE_PPC64_ASM', language : 'c')
  elif with_asm.enabled()
    error('No ASM available for @0@ (@1@ endian)'.format(host_machine.system(), host_machine.endian()))
  else
    use_asm = false
  endif
endif

dep_dl = cc.find_library('dl', required : false)
dep_m = cc.find_library('m', required : false)
dep_threads = dependency('threads')
dep_x11 = dependency('x11', required : get_option('x11'))

dep_x11_headers = dep_x11.partial_dependency(compile_args : true, includes : true)
if dep_x11.found()
  add_project_arguments('-DENABLE_EGL_X11', language : ['c'])
endif

dep_xext = dep_null
dep_glproto = dep_null
with_glx = false
if get_option('glx').enabled() and not dep_x11.found()
  error('Cannot build GLX support without X11.')
elif not get_option('glx').disabled() and dep_x11.found()
  dep_xext = dependency('xext', required : get_option('glx'))
  dep_glproto = dependency('glproto', required : get_option('glx'))
  with_glx = true
endif

with_hgl = false
if get_option('hgl') and host_machine.system() in ['haiku']
  add_languages('cpp')
  with_hgl = true
endif

if cc.compiles('typeof(int *);', name : 'typeof')
  add_project_arguments('-DHAVE_TYPEOF', language : ['c'])
endif

with_tls = get_option('tls')
if with_tls
  # Use a __thread variable if possible, even if we can't use the TLS assembly
  # stubs, because it's still faster than calling pthread_getspecific.
  have_tls = cc.compiles(
    '__thread int foo;',
    name : '__thread',
  )
else
  have_tls = false
endif
message('Using TLS variable for dispatch table: @0@'.format(have_tls))

if have_tls
  add_project_arguments('-DGLDISPATCH_USE_TLS', language : ['c'])
endif

gl_dispatch_type = 'pure_c'
if use_asm
  # TLS asm requires initial-exec, which is not portable (only supported by
  # FreeBSD and glibc). on other platforms we use TSD stubs to allow calling
  # extension functions unknown at compile time.
  # https://gitlab.freedesktop.org/glvnd/libglvnd/-/merge_requests/249
  thread_type = 'tsd'
  if get_option('dispatch-tls')
    if have_tls
      if host_machine.system() == 'freebsd' or cc.has_header_symbol('features.h', '__GLIBC__')
        thread_type = 'tls'
      endif
    endif
  endif

  if host_machine.cpu_family().startswith('x86')
    gl_dispatch_type = '@0@_@1@'.format(
      host_machine.cpu_family(),
      thread_type,
    )
  elif host_machine.cpu_family() == 'arm'
    gl_dispatch_type = 'armv7_tsd'
  elif host_machine.cpu_family() == 'aarch64'
    gl_dispatch_type = 'aarch64_tsd'
  elif host_machine.cpu_family() == 'ppc64'
    gl_dispatch_type = 'ppc64_@0@'.format(thread_type)
  endif
  add_project_arguments('-DUSE_DISPATCH_ASM', language : 'c')
endif
message('Using dispatch stub type: @0@'.format(gl_dispatch_type))

# -mtls-dialect=gnu2 speeds up non-initial-exec TLS significantly but requires
# full toolchain (including libc) support.
#
# tls stubs are incompatible with tlsdesc, but are only compatible with
# initial-exec which is faster anyways
if not gl_dispatch_type.endswith('_tls')
  have_mtls_dialect = false
  foreach c_arg : get_option('c_args')
    if c_arg.startswith('-mtls-dialect=')
      have_mtls_dialect = true
      break
    endif
  endforeach
  if not have_mtls_dialect
    # need .run to check libc support. meson aborts when calling .run when
    # cross-compiling, but because this is just an optimization we can skip it
    if meson.is_cross_build()
      warning('cannot auto-detect -mtls-dialect when cross-compiling, using compiler default')
    else
      # -fpic to force dynamic tls, otherwise TLS relaxation defeats check
      gnu2_test = cc.run('int __thread x; int main() { return x; }', args: ['-mtls-dialect=gnu2', '-fpic'], name: '-mtls-dialect=gnu2')
      if gnu2_test.returncode() == 0
        add_project_arguments('-mtls-dialect=gnu2', language : ['c'])
      endif
    endif
  endif
endif

if cc.has_function_attribute('constructor')
  add_project_arguments('-DUSE_ATTRIBUTE_CONSTRUCTOR', language : ['c'])
endif

if cc.compiles('''
    #include <pthread.h>
    void foo(void)
    {
      pthread_rwlock_t lock;
      pthread_rwlock_init(&lock, NULL);
    }''',
    name : 'pthread rwlock')
  add_project_arguments('-DHAVE_PTHREAD_RWLOCK', language : ['c'])
endif

if cc.compiles('''
    int foo(int volatile *val, int oldVal, int newVal)
    {
      return __sync_add_and_fetch(val, 1);
      return __sync_lock_test_and_set(val, newVal);
      return __sync_val_compare_and_swap(val, oldVal, newVal);
    }''',
    name : 'sync intrinsics')
  add_project_arguments('-DHAVE_SYNC_INTRINSICS', language : ['c'])
endif

if cc.has_function('mincore')
  add_project_arguments('-DHAVE_MINCORE', language : ['c'])
endif

if cc.has_header_symbol('dlfcn.h', 'RTLD_NOLOAD')
  add_project_arguments('-DHAVE_RTLD_NOLOAD', language : ['c'])
endif

if cc.has_member('struct dirent', 'd_type', prefix : '#include <dirent.h>')
  add_project_arguments('-DHAVE_DIRENT_DTYPE', language : ['c'])
endif

_p = get_option('dispatch-page-size')
if _p != 0
  add_project_arguments('-DGLDISPATCH_PAGE_SIZE=' + _p, language : ['c'])
endif

if not get_option('entrypoint-patching').disabled()
  add_project_arguments('-DGLDISPATCH_ENABLE_PATCHING', language : ['c'])
endif

# Set EGL_NO_X11 unconditionally, Libglvnd doesn't make any assumptions about
# native display or drawable types, so we don't need X11-specific typedefs for
# them
add_project_arguments('-DEGL_NO_X11', language : ['c'])

pkg = import('pkgconfig')

subdir('include')
subdir('src')
subdir('tests')

pkg.generate(
  name : 'libglvnd',
  description : 'Vendor-neutral OpenGL dispatch library vendor interface',
  version : meson.project_version(),
  variables : [
    # Drivers can use these to know where to install the JSON files to tell
    # libglvnd to load them. Meson doesn't have a separate datarootdir option,
    # so juse use datadir for both of these.
    'datarootdir=${prefix}/' + get_option('datadir'),
    'datadir=${prefix}/' + get_option('datadir'),
  ]
)

