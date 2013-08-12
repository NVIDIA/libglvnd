#!/bin/bash

export __GLX_VENDOR_LIBRARY_NAME=dummy
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TOP_BUILDDIR/tests/GLX_dummy/.libs

# We require pthreads be loaded before libGLX for correctness
export LD_PRELOAD=libpthread.so.0

# Run the make current test in a loop in multiple threads.
./testglxmakecurrent -t 5 -i 20000
