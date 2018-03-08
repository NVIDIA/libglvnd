noinst_HEADERS = \
	glvnd_genentry.h \
	utils_misc.h \
	glvnd_pthread.h \
	app_error_check.h \
	winsys_dispatch.h \
	trace.h \
	cJSON.h

EXTRA_DIST = uthash cJSON

noinst_LTLIBRARIES =
AM_CPPFLAGS = -I$(top_srcdir)/include

noinst_LTLIBRARIES += libglvnd_pthread.la
libglvnd_pthread_la_LIBADD = @LIB_DL@
libglvnd_pthread_la_SOURCES = glvnd_pthread.c

noinst_LTLIBRARIES += libapp_error_check.la
libapp_error_check_la_SOURCES = app_error_check.c

noinst_LTLIBRARIES += libutils_misc.la
libutils_misc_la_SOURCES = utils_misc.c

noinst_LTLIBRARIES += libglvnd_genentry.la
libglvnd_genentry_la_SOURCES = glvnd_genentry.c

noinst_LTLIBRARIES += libtrace.la
libtrace_la_SOURCES = trace.c

noinst_LTLIBRARIES += libwinsys_dispatch.la
libwinsys_dispatch_la_SOURCES = winsys_dispatch.c
libwinsys_dispatch_la_CFLAGS = -I$(top_srcdir)/src/util/uthash/src

noinst_LTLIBRARIES += libcJSON.la
libcJSON_la_SOURCES = cJSON.c
