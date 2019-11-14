#!/bin/bash

set -e
set -o xtrace

# Running a unity build (sometimes called a jumbo build) is both a useful thing
# to test and reduces compile time.
#
# Enable all auto-features to ensure that we're proprely testing all optional
# dependencies.
meson build --unity=on --auto-features=enabled $CONFIGURE_OPTIONS
ninja -C build

Xvfb :99 &

set +e
DISPLAY=:99 ninja -C build test
RESULT=$?
set -e

kill %Xvfb
exit $RESULT
