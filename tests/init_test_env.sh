#!/bin/bash

echo -n "Initializing test environment... "

if [ -z "$DO_X11_TESTS" ]; then
    echo "skipped"
    exit 77
fi
echo "(unset DO_X11_TESTS to disable this step)"

X -config "$ABS_TOP_BUILDDIR/tests/xorg.2screens.conf" \
  -modulepath "/usr/lib/xorg/modules/,$ABS_TOP_BUILDDIR/src/x11glvnd/.libs" \
  -retro -keeptty -noreset \
  $DISPLAY &> /dev/null &

echo $! > xorg.pid

sleep 5

exit 0
