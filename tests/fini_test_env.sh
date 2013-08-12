#!/bin/sh

if [ -n "$SKIP_ENV_INIT" ]; then
    echo "Test environment cleanup skipped"
    exit 77
fi

echo "Cleaning up test environment"

XORG_PID=$(cat xorg.pid)

kill $XORG_PID

rm xorg.pid

exit 0
