#!/bin/bash

export __GLX_VENDOR_LIBRARY_NAME=dummy
export GLVND_TEST_PATCH_ENTRYPOINTS=1
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TOP_BUILDDIR/tests/GLX_dummy/.libs

# Run the patch entrypoint test.
./testpatchentrypoints
