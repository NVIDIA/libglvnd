#!/bin/bash

export __GLX_VENDOR_LIBRARY_NAME=dummy
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TOP_BUILDDIR/tests/GLX_dummy/.libs


# Run the make current test exactly once, but with GetProcAddress() called
# after MakeCurrent().
./testglxmakecurrent -t 1 -i 1 -l
