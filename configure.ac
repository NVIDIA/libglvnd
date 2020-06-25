dnl configure.ac
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([libglvnd], [1.3.2], [kbrenneman@nvidia.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([bin])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM

dnl Add an --enable-debug option
AX_CHECK_ENABLE_DEBUG(no, DEBUG)

AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.11 foreign silent-rules])
AM_SILENT_RULES([yes])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_AS
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG
AC_PROG_NM

AC_ARG_ENABLE([werror],
    [AS_HELP_STRING([--enable-werror],
        [Build with -Werror @<:@default=disabled@:>@])],
    [enable_werror=$enableval],
    [enable_werror=no])
if test "x$enable_werror" = xyes; then
    CFLAGS="$CFLAGS -Werror"
fi

# The tarball from "make dist" already contains all of the generated files. If
# we're building from that, then we won't need Python.
if test -e "$srcdir/src/GLdispatch/vnd-glapi/g_glapi_mapi_gl_tmp.h" ; then
    AM_PATH_PYTHON([2.7],, [:])
else
    AM_PATH_PYTHON([2.7])
fi
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])

if test "x$ac_cv_prog_cc_c99" = xno; then
        AC_MSG_ERROR([Building libglvnd requires a C99-enabled compiler])
fi

AC_ARG_ENABLE([egl],
    [AS_HELP_STRING([--disable-egl],
        [Disable EGL support @<:@default=enabled@:>@])],
    [enable_egl="$enableval"],
    [enable_egl=yes]
)
AM_CONDITIONAL([ENABLE_EGL], [test "x$enable_egl" = "xyes"])

AC_ARG_ENABLE([x11],
    [AS_HELP_STRING([--disable-x11],
        [Disable X11 support. Implies --disable-glx @<:@default=enabled@:>@])],
    [enable_x11="$enableval"],
    [enable_x11=yes]
)
AM_CONDITIONAL([ENABLE_X11], [test "x$enable_x11" = "xyes"])

AC_ARG_ENABLE([glx],
    [AS_HELP_STRING([--disable-glx],
        [Disable GLX support @<:@default=enabled@:>@])],
    [enable_glx="$enableval"],
    [enable_glx="$enable_x11"]
)
AM_CONDITIONAL([ENABLE_GLX], [test "x$enable_glx" = "xyes"])

if test "x$enable_x11" != "xyes" -a "x$enable_glx" = "xyes" ; then
    AC_MSG_ERROR([Can't build GLX without X11.])
fi

AC_ARG_ENABLE([gles1],
    [AS_HELP_STRING([--disable-gles1],
        [disable support for OpenGL ES 1.x API @<:@default=enabled@:>@])],
    [enable_gles1="$enableval"],
    [enable_gles1=yes])
AM_CONDITIONAL([ENABLE_GLES1], [test "x$enable_gles1" = "xyes"])

AC_ARG_ENABLE([gles2],
    [AS_HELP_STRING([--disable-gles2],
        [disable support for OpenGL ES 2.x and 3.x API @<:@default=enabled@:>@])],
    [enable_gles2="$enableval"],
    [enable_gles2=yes])
AM_CONDITIONAL([ENABLE_GLES2], [test "x$enable_gles2" = "xyes"])

AC_ARG_ENABLE([headers],
    [AS_HELP_STRING([--disable-headers],
        [Do not install the GL/GLES/GLX/EGL header files @<:@default=enabled@:>@])],
    [enable_headers="$enableval"],
    [enable_headers=yes]
)
AM_CONDITIONAL([ENABLE_GL_HEADERS], [test "x$enable_headers" = "xyes"])
AM_CONDITIONAL([ENABLE_EGL_HEADERS], [test "x$enable_headers" = "xyes" -a "x$enable_egl" = "xyes"])
AM_CONDITIONAL([ENABLE_GLX_HEADERS], [test "x$enable_headers" = "xyes" -a "x$enable_glx" = "xyes"])
AM_CONDITIONAL([ENABLE_GLES1_HEADERS], [test "x$enable_headers" = "xyes" -a "x$enable_gles1" = "xyes"])
AM_CONDITIONAL([ENABLE_GLES2_HEADERS], [test "x$enable_headers" = "xyes" -a "x$enable_gles2" = "xyes"])

dnl
dnl Arch/platform-specific settings. Copied from mesa
dnl
AC_ARG_ENABLE([asm],
    [AS_HELP_STRING([--disable-asm],
        [disable assembly usage @<:@default=enabled on supported platforms@:>@])],
    [enable_asm="$enableval"],
    [enable_asm=yes]
)

asm_arch=""
AC_MSG_CHECKING([whether to enable assembly])
test "x$enable_asm" = xno && AC_MSG_RESULT([no])
# check for supported arches
if test "x$enable_asm" = xyes; then
    case "$host_cpu" in
    i?86)
        case "$host_os" in
        linux* | *freebsd* | dragonfly* | *netbsd*)
            asm_arch=x86
            ;;
        gnu*)
            asm_arch=x86
            ;;
        esac
        ;;
    x86_64 | amd64)
        case "$host_os" in
        linux* | *freebsd* | dragonfly* | *netbsd*)
            asm_arch=x86_64
            ;;
        esac
        ;;
    armv7*l | armv8*l)
        asm_arch=armv7l
        ;;
    aarch64)
        asm_arch=aarch64
        ;;
    powerpc64le)
        asm_arch=ppc64
        ;;
    powerpc64)
        AC_COMPILE_IFELSE([AC_LANG_SOURCE([
        #if !defined(_CALL_ELF) || (_CALL_ELF) == 1
        #  error "ELFv1 ABI"
        #endif
        ])],
        [asm_arch=ppc64],[])
    esac

    case "$asm_arch" in
    x86)
        DEFINES="$DEFINES -DUSE_X86_ASM"
        AC_MSG_RESULT([yes, x86])
        ;;
    x86_64)
        DEFINES="$DEFINES -DUSE_X86_64_ASM"
        AC_MSG_RESULT([yes, x86_64])
        ;;
    armv7l)
        DEFINES="$DEFINES -DUSE_ARMV7_ASM"
        AC_MSG_RESULT([yes, armv7l])
        ;;
    aarch64)
        DEFINES="$DEFINES -DUSE_AARCH64_ASM"
        AC_MSG_RESULT([yes, aarch64])
        ;;
    ppc64)
        DEFINES="$DEFINES -DUSE_PPC64_ASM"
        AC_MSG_RESULT([yes, ppc64])
        ;;
    *)
        AC_MSG_RESULT([no, platform '$host_cpu' not supported])
        ;;
    esac
fi

dnl
dnl mapi top-relative paths: defined here so mapi can be used elsewhere
dnl

dnl Checks for libraries.
AX_PTHREAD()

if test "x$enable_x11" = "xyes" ; then
    PKG_CHECK_MODULES([X11], [x11])
    AC_DEFINE([USE_X11], 1,
        [Define to 1 if X11 support is enabled.])
fi
if test "x$enable_glx" = "xyes" ; then
    PKG_CHECK_MODULES([XEXT], [xext])
    PKG_CHECK_MODULES([GLPROTO], [glproto])
fi

AS_IF([test "x$gldispatch_use_tls" = "xyes"],
      [AC_DEFINE([GLDISPATCH_USE_TLS], 1,
      [Define to 1 if libGLdispatch should use a TLS variable for the dispatch table.])])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_TYPEOF

dnl Checks for library functions.
AC_FUNC_STRNLEN

dnl TLS detection
AC_ARG_ENABLE([tls],
    [AS_HELP_STRING([--disable-tls],
        [disable TLS usage @<:@default=enabled on supported platforms@:>@])],
    [enable_tls="$enableval"],
    [enable_tls=yes]
)

AC_MSG_CHECKING([for initial-exec TLS])
if test "x$enable_tls" = "xyes"; then
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
       __thread int foo __attribute__((tls_model("initial-exec")));
    ])],
    [HAVE_INIT_TLS=yes],[HAVE_INIT_TLS=no])
else
    HAVE_INIT_TLS=no
fi
AC_MSG_RESULT($HAVE_INIT_TLS)

# Figure out what implementation to use for the entrypoint stubs.
# This will set an automake condition, which is then used in
# src/GLdispatch/vnd-glapi/entry_files.mk.

AC_MSG_CHECKING([for entrypoint stub type])
case "x$asm_arch" in
xx86 | xx86_64)
    # For x86 and x86-64, both the TLS and TSD stubs work.
    if test "x$HAVE_INIT_TLS" = "xyes" ; then
        gldispatch_entry_type=${asm_arch}_tls
        gldispatch_use_tls=yes
    else
        gldispatch_entry_type=${asm_arch}_tsd
        gldispatch_use_tls=no
    fi
    ;;
xarmv7l)
    # For ARMv7, only the TSD stubs have been implemented yet.
    gldispatch_entry_type=armv7_tsd
    gldispatch_use_tls=no
    ;;
xaarch64)
    # For aarch64, only the TSD stubs have been implemented yet.
    gldispatch_entry_type=aarch64_tsd
    gldispatch_use_tls=no
    ;;
xppc64)
    # For ppc64, allow both the TLS and TSD stubs for now.
    if test "x$HAVE_INIT_TLS" = "xyes" ; then
        gldispatch_entry_type=ppc64_tls
        gldispatch_use_tls=yes
    else
        gldispatch_entry_type=ppc64_tsd
        gldispatch_use_tls=no
    fi
    ;;
*)
    # The C stubs will work with either TLS or TSD.
    gldispatch_entry_type=pure_c
    gldispatch_use_tls=$HAVE_INIT_TLS
    ;;
esac
AC_MSG_RESULT([$gldispatch_entry_type, TLS=$gldispatch_use_tls])

AS_IF([test "x$gldispatch_use_tls" = "xyes"],
      [AC_DEFINE([GLDISPATCH_USE_TLS], 1,
      [Define to 1 if libGLdispatch should use a TLS variable for the dispatch table.])])
AM_CONDITIONAL([GLDISPATCH_USE_TLS], [test "x$gldispatch_use_tls" = "xyes"])
AM_CONDITIONAL([GLDISPATCH_TYPE_X86_TLS], [test "x$gldispatch_entry_type" = "xx86_tls"])
AM_CONDITIONAL([GLDISPATCH_TYPE_X86_TSD], [test "x$gldispatch_entry_type" = "xx86_tsd"])
AM_CONDITIONAL([GLDISPATCH_TYPE_X86_64_TLS], [test "x$gldispatch_entry_type" = "xx86_64_tls"])
AM_CONDITIONAL([GLDISPATCH_TYPE_X86_64_TSD], [test "x$gldispatch_entry_type" = "xx86_64_tsd"])
AM_CONDITIONAL([GLDISPATCH_TYPE_PPC64_TLS], [test "x$gldispatch_entry_type" = "xppc64_tls"])
AM_CONDITIONAL([GLDISPATCH_TYPE_PPC64_TSD], [test "x$gldispatch_entry_type" = "xppc64_tsd"])
AM_CONDITIONAL([GLDISPATCH_TYPE_ARMV7_TSD], [test "x$gldispatch_entry_type" = "xarmv7_tsd"])
AM_CONDITIONAL([GLDISPATCH_TYPE_AARCH64_TSD], [test "x$gldispatch_entry_type" = "xaarch64_tsd"])
AM_CONDITIONAL([GLDISPATCH_TYPE_PURE_C], [test "x$gldispatch_entry_type" = "xpure_c"])

AS_IF([test "x$gldispatch_entry_type" != "xpure_c"],
      [AC_DEFINE([USE_DISPATCH_ASM], 1,
      [Define to 1 if libGLdispatch and libGLX should use assembly dispatch functions.])])

AC_MSG_CHECKING([for constructor attributes])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
void __attribute__ ((constructor)) foo(void)
{
}
void __attribute__ ((destructor)) bar(void)
{
}
])],
[USE_ATTRIBUTE_CONSTRUCTOR=yes],[USE_ATTRIBUTE_CONSTRUCTOR=no])
AC_MSG_RESULT($USE_ATTRIBUTE_CONSTRUCTOR)
AS_IF([test "x$USE_ATTRIBUTE_CONSTRUCTOR" = "xyes"],
      [AC_DEFINE([USE_ATTRIBUTE_CONSTRUCTOR], 1,
      [Define to 1 if the compiler supports constructor attributes.])])

AC_MSG_CHECKING([for pthreads rwlocks])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <pthread.h>
void foo(void)
{
    pthread_rwlock_t lock;
    pthread_rwlock_init(&lock, NULL);
}
])],
[HAVE_PTHREAD_RWLOCK_T=yes],[HAVE_PTHREAD_RWLOCK_T=no])
AC_MSG_RESULT($HAVE_PTHREAD_RWLOCK_T)
AS_IF([test "x$HAVE_PTHREAD_RWLOCK_T" = "xyes"],
      [AC_DEFINE([HAVE_PTHREAD_RWLOCK_T], 1,
      [Define to 1 if the compiler supports pthreads rwlocks.])])

AC_MSG_CHECKING([for sync intrinsics])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
int foo(int volatile *val, int oldVal, int newVal)
{
    return __sync_add_and_fetch(val, 1);
    return __sync_lock_test_and_set(val, newVal);
    return __sync_val_compare_and_swap(val, oldVal, newVal);
}
])],
[HAVE_SYNC_INTRINSICS=yes],[HAVE_SYNC_INTRINSICS=no])
AC_MSG_RESULT($HAVE_SYNC_INTRINSICS)
AS_IF([test "x$HAVE_SYNC_INTRINSICS" = "xyes"],
      [AC_DEFINE([HAVE_SYNC_INTRINSICS], 1,
      [Define to 1 if the compiler supports __sync intrinsic functions.])])

AC_CHECK_FUNC(mincore, [AC_DEFINE([HAVE_MINCORE], [1],
    [Define to 1 if mincore is available.])])

AC_CHECK_FUNC(dlopen, [],
    [AC_SUBST([LIB_DL], [-ldl])])

AC_MSG_CHECKING([for RTLD_NOLOAD])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <dlfcn.h>
void foo(void)
{
    (void) RTLD_NOLOAD;
}
])],
[HAVE_RTLD_NOLOAD=yes],[HAVE_RTLD_NOLOAD=no])
AC_MSG_RESULT($HAVE_RTLD_NOLOAD)
AS_IF([test "x$HAVE_RTLD_NOLOAD" = "xyes"],
      [AC_DEFINE([HAVE_RTLD_NOLOAD], 1,
      [Define to 1 if the compiler supports RTLD_NOLOAD.])])

AC_MSG_CHECKING([for dirent.d_type])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <dirent.h>
void foo(struct dirent *ent)
{
    (void) ent->d_type;
}
])],
[HAVE_DIRENT_DTYPE=yes],[HAVE_DIRENT_DTYPE=no])
AC_MSG_RESULT($HAVE_DIRENT_DTYPE)
AS_IF([test "x$HAVE_DIRENT_DTYPE" = "xyes"],
      [AC_DEFINE([HAVE_DIRENT_DTYPE], 1,
      [Define to 1 if struct dirent has a d_type member.])])


# See if the linker supports the --no-undefined flag.
AX_CHECK_LINK_FLAG([-Xlinker --no-undefined],
      [AC_SUBST([LINKER_FLAG_NO_UNDEFINED], ["-Xlinker --no-undefined"])],
      [AC_SUBST([LINKER_FLAG_NO_UNDEFINED], [""])])

AC_ARG_VAR([GLDISPATCH_PAGE_SIZE],
    [Page size to align static dispatch stubs])
AS_IF([test "x$GLDISPATCH_PAGE_SIZE" != "x"],
      [AC_DEFINE_UNQUOTED([GLDISPATCH_PAGE_SIZE], [$GLDISPATCH_PAGE_SIZE],
      [Page size to align static dispatch stubs.])])

# Set EGL_NO_X11 unconditionally. Libglvnd doesn't make any assumptions about
# native display or drawable types, so we don't need X11-specific typedefs for
# them.
DEFINES="$DEFINES -DEGL_NO_X11"

dnl default CFLAGS
CFLAGS="$CFLAGS -Wall -include config.h -fvisibility=hidden $DEFINES"

AC_CONFIG_FILES([Makefile
                 libglvnd.pc
                 include/Makefile
                 src/Makefile
                 src/GL/Makefile
                 src/GL/gl.pc
                 src/OpenGL/opengl.pc
                 src/OpenGL/Makefile
                 src/GLESv1/glesv1_cm.pc
                 src/GLESv1/Makefile
                 src/GLESv2/glesv2.pc
                 src/GLESv2/Makefile
                 src/GLX/glx.pc
                 src/GLX/Makefile
                 src/EGL/Makefile
                 src/EGL/egl.pc
                 src/GLdispatch/Makefile
                 src/GLdispatch/vnd-glapi/Makefile
                 src/util/Makefile
                 tests/Makefile
                 tests/dummy/Makefile])
AC_OUTPUT
