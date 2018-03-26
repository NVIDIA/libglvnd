#!/bin/sh

. $TOP_SRCDIR/tests/glxenv.sh

# Run the make current test exactly once, but with GetProcAddress() called
# after MakeCurrent().
./testglxmakecurrent -t 1 -i 1 -l
