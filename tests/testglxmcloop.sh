#!/bin/sh

. $TOP_SRCDIR/tests/glxenv.sh

# Run the make current test in a loop using a single thread.
./testglxmakecurrent -t 1 -i 250
