#!/bin/bash

set -e
set -o xtrace

# Running a unity build (sometimes called a jumbo build) is both a useful thing
# to test and reduces compile time.
#
# Enable all auto-features to ensure that we're properly testing all optional
# dependencies.
meson build -Dwerror=true --unity=on --auto-features=enabled $CONFIGURE_OPTIONS
ninja -C build

xvfb-run --auto-servernum ninja -C build test
