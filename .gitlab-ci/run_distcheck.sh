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

kill %Xvfb
exit $RESULT
