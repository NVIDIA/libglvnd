#!/bin/sh

. $TOP_SRCDIR/tests/glxenv.sh
export GLVND_TEST_PATCH_ENTRYPOINTS=1

# Run the patch entrypoint test.
./testpatchentrypoints
