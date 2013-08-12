#!/bin/bash

echo -n "Initializing test environment... "

if [ -n "$SKIP_ENV_INIT" ]; then
    echo "skipped"
    exit 77
fi
echo "(set SKIP_ENV_INIT to disable this step)"

X -config "$ABS_TOP_BUILDDIR/tests/xorg.2screens.conf" \
  -modulepath "/usr/lib/xorg/modules/,$ABS_TOP_BUILDDIR/src/x11glvnd/.libs" \
  -retro -keeptty -noreset \
  $DISPLAY &> /dev/null &

echo $! > xorg.pid

sleep 5

exit 0
