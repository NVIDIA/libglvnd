#!/bin/bash

set -e
set -o xtrace

./autogen.sh

mkdir build
cd build
../configure

Xvfb :99 &

set +e
DISPLAY=:99 make distcheck V=1 VERBOSE=1
RESULT=$?
set -e

# If make distcheck failed don't even bother with the meson check, the tarball
# may be invalid and it's just a waste
if [ $RESULT -ne 0 ]; then
    kill %Xvfb
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
set +e
DISPLAY=:99 ninja -C builddir test
RESULT=$?
set -e
popd

kill %Xvfb
exit $RESULT
