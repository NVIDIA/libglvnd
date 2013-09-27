#!/bin/bash

if [ -z "$DO_X11_TESTS" ]; then
    echo "Skipping test; requires environment init"
    exit 77
fi
./testx11glvndproto
