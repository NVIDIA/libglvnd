#!/bin/sh

. $TOP_SRCDIR/tests/glxenv.sh

# Run the make current test in a loop in multiple threads.
./testglxmakecurrent_mt -t 5 -i 20000
