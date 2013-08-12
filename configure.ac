dnl configure.ac
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([libglvnd], [0.0.0], [brnguyen@nvidia.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([bin])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM

AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([foreign])

dnl Checks for programs.
AC_PROG_AWK
AC_PROG_CXX

AC_PROG_CC
AM_PROG_AS
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_CHECK_PROGS([PYTHON2], [python2 python])

dnl
dnl Arch/platform-specific settings. Copied from mesa
dnl
AC_ARG_ENABLE([asm],
    [AS_HELP_STRING([--disable-asm],
        [disable assembly usage @<:@default=enabled on supported plaforms@:>@])],
    [enable_asm="$enableval"],
    [enable_asm=yes]
)
asm_arch=""
AC_MSG_CHECKING([whether to enable assembly])
test "x$enable_asm" = xno && AC_MSG_RESULT([no])
# disable if cross compiling on x86/x86_64 since we must run gen_matypes
if test "x$enable_asm" = xyes && test "x$cross_compiling" = xyes; then
    case "$host_cpu" in
    i?86 | x86_64)
        enable_asm=no
        AC_MSG_RESULT([no, cross compiling])
        ;;
    esac
fi
# check for supported arches
if test "x$enable_asm" = xyes; then
    case "$host_cpu" in
    i?86)
        case "$host_os" in
        linux* | *freebsd* | dragonfly* | *netbsd*)
            test "x$enable_64bit" = xyes && asm_arch=x86_64 || asm_arch=x86
            ;;
        gnu*)
            asm_arch=x86
            ;;
        esac
        ;;
    x86_64)
        case "$host_os" in
        linux* | *freebsd* | dragonfly* | *netbsd*)
            test "x$enable_32bit" = xyes && asm_arch=x86 || asm_arch=x86_64
            ;;
        esac
        ;;
    sparc*)
        case "$host_os" in
        linux*)
            asm_arch=sparc
            ;;
        esac
        ;;
    esac

    case "$asm_arch" in
    x86)
        DEFINES="$DEFINES -DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM"
        AC_MSG_RESULT([yes, x86])
        ;;
    x86_64)
        DEFINES="$DEFINES -DUSE_X86_64_ASM"
        AC_MSG_RESULT([yes, x86_64])
        ;;
    sparc)
        DEFINES="$DEFINES -DUSE_SPARC_ASM"
        AC_MSG_RESULT([yes, sparc])
        ;;
    *)
        AC_MSG_RESULT([no, platform not supported])
        ;;
    esac
fi

dnl Various conditionals.
AM_CONDITIONAL([GCC], [test x$GCC = xyes ])

AM_CONDITIONAL(HAVE_VND_GLAPI, true)
dnl HACK FIXME TODO
AM_CONDITIONAL(HAVE_SHARED_GLAPI, false)
AM_CONDITIONAL(HAVE_OPENGL, false)
AM_CONDITIONAL(HAVE_OPENGL_ES1, false)
AM_CONDITIONAL(HAVE_OPENGL_ES2, false)
AM_CONDITIONAL(HAVE_OPENVG, false)

dnl
dnl mapi top-relative paths: defined here so mapi can be used elsewhere
dnl

AC_SUBST([MAPI_PREFIX], [src/GLdispatch/mapi])
AC_SUBST([MAPI_MESA_PREFIX], [src/GLdispatch/mesa])

AM_CONDITIONAL(HAVE_X86_ASM, echo "$DEFINES" | grep 'X86_ASM' >/dev/null 2>&1)
AM_CONDITIONAL(HAVE_X86_64_ASM, echo "$DEFINES" | grep 'X86_64_ASM' >/dev/null 2>&1)
AM_CONDITIONAL(HAVE_SPARC_ASM, echo "$DEFINES" | grep 'SPARC_ASM' >/dev/null 2>&1)

dnl Checks for libraries.
AX_PTHREAD([true])

PKG_CHECK_MODULES([X11], [x11])
PKG_CHECK_MODULES([XEXT], [xext])
PKG_CHECK_MODULES([XORG], [xorg-server >= 1.11.0])

dnl Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h inttypes.h stddef.h stdlib.h string.h sys/param.h sys/time.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_TYPEOF
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([getpagesize gettimeofday memmove memset strdup strerror])

dnl TLS detection
AC_MSG_CHECKING([for initial-exec TLS])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
   __thread int foo __attribute__((tls_model("initial-exec")));
])],
[GLX_USE_TLS=yes],[GLX_USE_TLS=no])

AC_MSG_RESULT($GLX_USE_TLS)
AS_IF([test "x$GLX_USE_TLS" = "xyes"],
      [AC_DEFINE([GLX_USE_TLS], 1,
      [Define to 1 if ELF TLS with initial-exec addressing is available.])])

dnl default CFLAGS
CFLAGS="$CFLAGS -Wall -Werror -std=gnu99 -include config.h -fvisibility=hidden $DEFINES"

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/GLX/Makefile
                 src/x11glvnd/Makefile
                 src/GLdispatch/Makefile
                 src/GLdispatch/mapi/Makefile
                 src/GLdispatch/mapi/glapi/Makefile
                 src/GLdispatch/mapi/glapi/gen/Makefile
                 src/GLdispatch/mapi/vnd-glapi/Makefile
                 src/util/glvnd_pthread/Makefile
                 src/util/trace/Makefile])
AC_OUTPUT
