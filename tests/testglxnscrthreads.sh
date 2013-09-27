#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TOP_BUILDDIR/tests/GLX_dummy/.libs

# We require pthreads be loaded before libGLX for correctness
export LD_PRELOAD=libpthread.so.0

if [ -z "$DO_X11_TESTS" ]; then
    echo "Skipping test; requires environment init"
    exit 77
fi

./testglxnscreens -i 100 -t 5
