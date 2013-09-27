#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TOP_BUILDDIR/tests/GLX_dummy/.libs

if [ -z "$DO_X11_TESTS" ]; then
    echo "Skipping test; requires environment init"
    exit 77
fi

./testglxnscreens -i 100
