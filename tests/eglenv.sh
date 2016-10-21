#!/bin/bash

export __EGL_VENDOR_LIBRARY_DIRS=$TOP_SRCDIR/tests/json
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TOP_BUILDDIR/tests/dummy/.libs
