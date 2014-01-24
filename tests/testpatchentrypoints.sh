#!/bin/bash

export __GLX_VENDOR_LIBRARY_NAME=patchentry
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TOP_BUILDDIR/tests/GLX_dummy/.libs

# Run the patch entrypoint test.
./testpatchentrypoints
