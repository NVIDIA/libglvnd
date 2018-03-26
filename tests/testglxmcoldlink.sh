#!/bin/sh

. $TOP_SRCDIR/tests/glxenv.sh

# Run the make current test exactly once.
./testglxmakecurrent_oldlink -t 1 -i 1
