#!/bin/bash

if [ -n "$SKIP_ENV_INIT" ]; then
    echo "Skipping test; requires environment init"
    exit 77
fi
./testx11glvndproto
