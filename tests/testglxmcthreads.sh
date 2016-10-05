#!/bin/bash

source $TOP_SRCDIR/tests/glxenv.sh

# We require pthreads be loaded before libGLX for correctness
export LD_PRELOAD=libpthread.so.0

# Run the make current test in a loop in multiple threads.
./testglxmakecurrent -t 5 -i 20000
