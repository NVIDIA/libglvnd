#!/bin/sh

test "${PYTHON}" = ":" && exit 77
test "x${NM}" = "x" && exit 77

exec "${PYTHON}" "${TOP_SRCDIR}/bin/symbols-check.py" \
    --nm "${NM}" \
    --lib "${TOP_BUILDDIR}/src/GLESv1/.libs/libGLESv1_CM.so" \
    --symbols-file "${TOP_SRCDIR}/src/GLESv1/glesv1.symbols"
