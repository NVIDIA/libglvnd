# This fragment lists the files used for the dispatch stub implementation in
# libGLdispatch, libOpenGL, and libGL.
# The conditionals here are all set from the configure script.

if GLDISPATCH_TYPE_X86_TLS
MAPI_GLDISPATCH_ENTRY_FILES = mapi/entry_x86_tls.c
endif

if GLDISPATCH_TYPE_X86_TSD
MAPI_GLDISPATCH_ENTRY_FILES = mapi/entry_x86_tsd.c
MAPI_GLDISPATCH_ENTRY_FILES += mapi/entry_x86_64_common.c
MAPI_GLDISPATCH_ENTRY_FILES += mapi/entry_common.c
endif

if GLDISPATCH_TYPE_X86_64_TLS
MAPI_GLDISPATCH_ENTRY_FILES = mapi/entry_x86_64_tls.c
MAPI_GLDISPATCH_ENTRY_FILES += mapi/entry_x86_64_common.c
MAPI_GLDISPATCH_ENTRY_FILES += mapi/entry_common.c
endif

if GLDISPATCH_TYPE_X86_64_TSD
MAPI_GLDISPATCH_ENTRY_FILES = mapi/entry_x86_64_tsd.c
MAPI_GLDISPATCH_ENTRY_FILES += mapi/entry_x86_64_common.c
MAPI_GLDISPATCH_ENTRY_FILES += mapi/entry_common.c
endif


if GLDISPATCH_TYPE_ARMV7_TSD
MAPI_GLDISPATCH_ENTRY_FILES = mapi/entry_armv7_tsd.c
MAPI_GLDISPATCH_ENTRY_FILES += mapi/entry_common.c
endif

if GLDISPATCH_TYPE_PURE_C
MAPI_GLDISPATCH_ENTRY_FILES = mapi/entry_pure_c.c
endif

# vim:filetype=automake
