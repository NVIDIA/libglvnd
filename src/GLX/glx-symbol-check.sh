#!/bin/sh

test "${PYTHON}" = ":" && exit 77
test "x${NM}" = "x" && exit 77

exec "${PYTHON}" "${TOP_SRCDIR}/bin/symbols-check.py" \
    --nm "${NM}" \
    --lib "${TOP_BUILDDIR}/src/GLX/.libs/libGLX.so" \
    --symbols-file "${TOP_SRCDIR}/src/GLX/glx.symbols"
