#!/bin/sh

test "${PYTHON}" = ":" && exit 77
test "x${NM}" = "x" && exit 77

exec "${PYTHON}" "${TOP_SRCDIR}/bin/symbols-check.py" \
    --nm "${NM}" \
    --lib "${TOP_BUILDDIR}/src/GLESv2/.libs/libGLESv2.so" \
    --symbols-file "${TOP_SRCDIR}/src/GLESv2/glesv2.symbols"
