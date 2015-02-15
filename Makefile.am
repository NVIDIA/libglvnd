SUBDIRS = src tests
noinst_HEADERS = include
ACLOCAL_AMFLAGS = -I m4

if HAVE_XORG_SUBDIR
sysconfig_DATA = \
        10-x11glvnd.conf
endif

EXTRA_DIST = 10-x11glvnd.conf

AM_DISTCHECK_CONFIGURE_FLAGS = sysconfigdir=\$${datarootdir}/X11/xorg.conf.d
