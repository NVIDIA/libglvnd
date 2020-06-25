#!/bin/bash

set -e
set -o xtrace

./autogen.sh

mkdir build
cd build
../configure --enable-werror $CONFIGURE_OPTIONS

make V=1 VERBOSE=1

xvfb-run --auto-servernum make check V=1 VERBOSE=1

