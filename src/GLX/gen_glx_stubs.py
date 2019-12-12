#!/usr/bin/env python

import sys

GENERATED_ENTRYPOINT_MAX = 4096

def main():
    text = ""

    text += "#if defined(GLX_STUBS_COUNT)\n"
    text += "#define GENERATED_ENTRYPOINT_MAX %d\n" % (GENERATED_ENTRYPOINT_MAX)
    text += "#undef GLX_STUBS_COUNT\n"
    text += "#endif\n\n"

    text += "#if defined(GLX_STUBS_ASM)\n"
    for i in range(GENERATED_ENTRYPOINT_MAX):
        text += "STUB_ASM(\"%d\")\n" % (i)
    text += "#undef GLX_STUBS_ASM\n"
    text += "#endif\n"
    sys.stdout.write(text)

if (__name__ == "__main__"):
    main()

