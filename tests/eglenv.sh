#!/bin/sh

__EGL_VENDOR_LIBRARY_DIRS=$TOP_SRCDIR/tests/json
export __EGL_VENDOR_LIBRARY_DIRS

PLATFORM=$(uname -s)
if test $PLATFORM -eq "Haiku"; then
	LIBRARY_PATH=$LIBRARY_PATH:$TOP_BUILDDIR/tests/dummy/.libs
	export LIBRARY_PATH
else
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TOP_BUILDDIR/tests/dummy/.libs
	export LD_LIBRARY_PATH
fi
