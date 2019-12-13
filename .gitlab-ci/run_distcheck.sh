#!/bin/bash

set -e
set -o xtrace

./autogen.sh

mkdir build
cd build
../configure

xvfb-run --auto-servernum make distcheck V=1 VERBOSE=1

# If make distcheck failed don't even bother with the meson check, the tarball
# may be invalid and it's just a waste
if [ $RESULT -ne 0 ]; then
    exit $RESULT
fi

# Also check that the meson build works from the dist tarball

# We don't have a good way to know what the name of the archive will be (since
# it has the version in it). Therefore, write the tarball to a place we know
# the name of and work from there.
mkdir libglvnd
tar -xf libglvnd-*.tar.gz -C libglvnd --strip-components 1
pushd libglvnd
meson builddir --auto-features=enabled
xvfb-run --auto-servernum ninja -C builddir test
popd

