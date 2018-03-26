#!/bin/sh

. $TOP_SRCDIR/tests/glxenv.sh

# Set __GLX_FORCE_VENDOR_LIBRARY_0 instead of __GLX_VENDOR_LIBRARY_NAME. That
# way, it won't pre-load the vendor library, which would force it to generate
# a dispatch stub for glXExampleExtensionFunction.
__GLX_FORCE_VENDOR_LIBRARY_0=$__GLX_VENDOR_LIBRARY_NAME
export __GLX_FORCE_VENDOR_LIBRARY_0
unset __GLX_VENDOR_LIBRARY_NAME

./testglxgetprocaddress
