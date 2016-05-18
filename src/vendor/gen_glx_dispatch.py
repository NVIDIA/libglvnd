#!/usr/bin/python

# Copyright (c) 2016, NVIDIA CORPORATION.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# unaltered in all copies or substantial portions of the Materials.
# Any additions, deletions, or changes to the original source files
# must be clearly indicated in accompanying documentation.
#
# If only executable code is distributed, then the accompanying
# documentation must state that "this software is based in part on the
# work of the Khronos Group."
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

"""
Generates dispatch functions for GLX.

This script is provided to make it easier to generate the GLX dispatch stubs
for a vendor library.

The script will read a list of functions from Khronos's glx.xml list, with
additional information provided in a separate Python module.

See example_glx_function_list.py for an example of the function list.
"""

import sys
import collections
import imp

import os.path
import genCommon

_GLX_DRAWABLE_TYPES = frozenset(("GLXDrawable", "GLXWindow", "GLXPixmap",
        "GLXPbuffer", "GLXPbufferSGIX"))
_GLX_FBCONFIG_TYPES = frozenset(("GLXFBConfig", "GLXFBConfigSGIX"))

def main():
    if (len(sys.argv) < 4):
        print("Usage: %r source|header <function_list> <xml_file> [xml_file...]" % (sys.argv[0],))
        sys.exit(2)

    target = sys.argv[1]
    funcListFile = sys.argv[2]
    xmlFiles = sys.argv[3:]

    # The function list is a Python module, but it's specified on the command
    # line.
    glxFunctionList = imp.load_source("glxFunctionList", funcListFile)

    xmlFunctions = genCommon.getFunctions(xmlFiles)
    xmlByName = dict((f.name, f) for f in xmlFunctions)
    functions = []
    for (name, glxFunc) in glxFunctionList.GLX_FUNCTIONS:
        func = xmlByName[name]
        glxFunc = fixupFunc(func, glxFunc)
        functions.append((func, glxFunc))

    # Sort the function list by name.
    functions = sorted(functions, key=lambda f: f[0].name)

    if (target == "header"):
        text = generateHeader(functions)
    elif (target == "source"):
        text = generateSource(functions)
    else:
        raise ValueError("Invalid target: %r" % (target,))
    sys.stdout.write(text)

def fixupFunc(func, glxFunc):
    """
    Does some basic error-checking on a function, and fills in default values
    for any fields that haven't been set.
    """

    result = dict(glxFunc)
    if (result.get("prefix") == None):
        result["prefix"] = ""

    if (result.get("static") == None):
        result["static"] = False

    if (result.get("public") == None):
        result["public"] = False

    if (result.get("static") and result.get("public")):
        raise ValueError("Function %s cannot be both static and public" % (func.name,))

    # If the method is "custom" or "none", then we're not going to generate a
    # stub for it, so the rest of the data doesn't matter.
    if (result["method"] in ("custom", "none")):
        return result

    if (func.hasReturn()):
        if (result.get("retval") == None):
            result["retval"] = getDefaultReturnValue(func.rt)

    if (result.get("extension") != None):
        text = "defined(" + result["extension"] + ")"
        result["extension"] = text

    if (result["method"] != "current"):
        # This function is dispatched based on a paramter. Figure out which
        # one.
        if (result.get("key") != None):
            # The parameter was specified, so just use it.
            result["key"] = fixupParamName(func, result["key"])
        else:
            # Look for a parameter of the correct type.
            if (result["method"] == "drawable"):
                result["key"] = findParamByType(func, _GLX_DRAWABLE_TYPES)
            elif (result["method"] == "context"):
                result["key"] = findParamByType(func, ("GLXContext", "const GLXContext"))
            elif (result["method"] == "config"):
                result["key"] = findParamByType(func, _GLX_FBCONFIG_TYPES)
            elif (result["method"] == "screen"):
                result["key"] = fixupParamName(func, "screen")
            elif (result["method"] == "xvisinfo"):
                result["key"] = findParamByType(func, ("XVisualInfo *", "const XVisualInfo *"))
            else:
                raise ValueError("Unknown lookup method %r for function %r" % (result["method"], func.name))
            if (result["key"] == None):
                raise ValueError("Can't find lookup key for function %r" % (func.name,))

        if (result["method"] == "xvisinfo"):
            # If we're using an XVisualInfo pointer, then we just need the
            # screen number from it.
            result["key"] = "(" + result["key"] + ")->screen"
            result["method"] = "screen"

    if (result["method"] == "drawable"):
        # For reporting errors when we look up a vendor by drawable, we also
        # have to specify the error code.
        if (result.get("opcode") != None and result.get("error") == None):
            raise ValueError("Missing error code for function %r" % (func.name,))

    if (result.get("remove_mapping") != None):
        if (result["remove_mapping"] not in ("context", "drawable", "config")):
            raise ValueError("Invalid remove_mapping value %r for function %r" %
                    (result["remove_mapping"], func.name))
        if (result.get("remove_key") != None):
            result["remove_key"] = fixupParamName(func, result["remove_key"])
        else:
            assert(result["remove_mapping"] == result["method"])
            result["remove_key"] = result["key"]

    if (result.get("opcode") == None):
        result["opcode"] = "-1"
        result["error"] = "-1"

    if (result.get("display") != None):
        result["display"] = fixupParamName(func, result["display"])
    else:
        result["display"] = findParamByType(func, ("Display *",))
        if (result["display"] == None):
            if (getNeedsDisplayPointer(func, result)):
                raise ValueError("Can't find display pointer for function %r" % (func.name,))
            result["display"] = "NULL"

    return result

def getDefaultReturnValue(typename):
    """
    Picks a default return value. The dispatch stub will return this value if
    it can't find an implementation function.
    """
    if (typename.endswith("*")):
        return "NULL"
    if (typename == "GLXContext" or typename in _GLX_FBCONFIG_TYPES):
        return "NULL"
    if (typename in _GLX_DRAWABLE_TYPES):
        return "None"
    if (typename in ("XID", "GLXFBConfigID", "GLXContextID")):
        return "None"

    return "0"

def getNeedsDisplayPointer(func, glxFunc):
    """
    Returns True if a function needs a Display pointer.
    """
    if (glxFunc["method"] not in ("none", "custom", "context", "current")):
        return True
    if (glxFunc.get("add_mapping") != None):
        return True
    if (glxFunc.get("remove_mapping") != None):
        return True
    return False

def findParamByType(func, typeNames):
    """
    Looks for a parameter of a given data type. Returns the name of the
    parameter, or None if no matching parameter was found.
    """
    for arg in func.args:
        if (arg.type in typeNames):
            return arg.name
    return None

def fixupParamName(func, paramSpec):
    """
    Takes a parameter that was specified in the function list and returns a
    parameter name or a C expression.
    """
    try:
        # If the parameter is an integer, then treat it as a parameter index.
        index = int(paramSpec)
        return func.args[index]
    except ValueError:
        # Not an integer
        pass

    # If the string is contained in parentheses, then assume it's a valid C
    # expression.
    if (paramSpec.startswith("(") and paramSpec.endswith(")")):
        return paramSpec[1:-1]

    # Otherwise, assume the string is a parameter name.
    for arg in func.args:
        if (arg.name == paramSpec):
            return arg.name

    raise ValueError("Invalid parameter name %r in function %r" % (paramSpec, func))

def generateHeader(functions):
    text = r"""
#ifndef G_GLXDISPATCH_STUBS_H
#define G_GLXDISPATCH_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

""".lstrip("\n")

    text += "enum {\n"
    for (func, glxFunc) in functions:
        text += generateGuardBegin(func, glxFunc)
        text += "    __GLX_DISPATCH_" + func.name + ",\n"
        text += generateGuardEnd(func, glxFunc)
    text += "    __GLX_DISPATCH_COUNT\n"
    text += "};\n\n"

    for (func, glxFunc) in functions:
        if (glxFunc["inheader"]):
            text += generateGuardBegin(func, glxFunc)
            text += "{f.rt} {ex[prefix]}{f.name}({f.decArgs});\n".format(f=func, ex=glxFunc)
            text += generateGuardEnd(func, glxFunc)

    text += r"""
#ifdef __cplusplus
}
#endif
#endif // G_GLXDISPATCH_STUBS_H
"""
    return text

def generateSource(functions):
    text = r'''
#include "glxdispatchstubs.h"
#include "g_glxdispatchstubs.h"

#include <X11/Xlibint.h>
#include <GL/glxproto.h>

'''.lstrip("\n")

    for (func, glxFunc) in functions:
        if (glxFunc["method"] not in ("none", "custom")):
            text += generateGuardBegin(func, glxFunc)
            text += generateDispatchFunc(func, glxFunc)
            text += generateGuardEnd(func, glxFunc)

    text += "\n"
    text += "const char * const __GLX_DISPATCH_FUNC_NAMES[__GLX_DISPATCH_COUNT] = {\n"
    for (func, glxFunc) in functions:
        text += generateGuardBegin(func, glxFunc)
        text += '    "' + func.name + '",\n'
        text += generateGuardEnd(func, glxFunc)
    text += "};\n"

    text += "const __GLXextFuncPtr __GLX_DISPATCH_FUNCS[__GLX_DISPATCH_COUNT] = {\n"
    for (func, glxFunc) in functions:
        text += generateGuardBegin(func, glxFunc)
        if (glxFunc["method"] != "none"):
            text += "    (__GLXextFuncPtr) " + glxFunc["prefix"] + func.name + ",\n"
        else:
            text += "    NULL, // " + func.name + "\n"
        text += generateGuardEnd(func, glxFunc)
    text += "};\n"

    return text

def generateGuardBegin(func, glxFunc):
    ext = glxFunc.get("extension")
    if (ext != None):
        return "#if " + ext + "\n"
    else:
        return ""

def generateGuardEnd(func, glxFunc):
    if (glxFunc.get("extension") != None):
        return "#endif\n"
    else:
        return ""

def generateDispatchFunc(func, glxFunc):
    text = ""

    if (glxFunc["static"]):
        text += "static "
    elif (glxFunc["public"]):
        text += "PUBLIC "

    text += r"""{f.rt} {ex[prefix]}{f.name}({f.decArgs})
{{
    typedef {f.rt} (* _pfn_{f.name})({f.decArgs});
"""

    text += "    __GLXvendorInfo *_vendor;\n"
    text += "    _pfn_{f.name} _ptr_{f.name};\n"
    if (func.hasReturn()):
        text += "    {f.rt} _ret = {ex[retval]};\n"
    text += "\n"

    # Look up the vendor library
    text += "    _vendor = "
    if (glxFunc["method"] == "current"):
        text += "__glXDispatchApiExports->getCurrentDynDispatch();\n"
    elif (glxFunc["method"] == "screen"):
        text += "__glXDispatchApiExports->getDynDispatch({ex[display]}, {ex[key]});\n"
    elif (glxFunc["method"] == "drawable"):
        text += "__glxDispatchVendorByDrawable({ex[display]}, {ex[key]}, {ex[opcode]}, {ex[error]});\n"
    elif (glxFunc["method"] == "context"):
        text += "__glxDispatchVendorByContext({ex[display]}, {ex[key]}, {ex[opcode]});\n"
    elif (glxFunc["method"] == "config"):
        text += "__glxDispatchVendorByConfig({ex[display]}, {ex[key]}, {ex[opcode]});\n"
    else:
        raise ValueError("Unknown dispatch method: %r" % (glxFunc["method"],))

    # Look up and call the function.
    text += r"""
    if (_vendor != NULL) {{
        _ptr_{f.name} = (_pfn_{f.name})
            __glXDispatchApiExports->fetchDispatchEntry(_vendor, __glXDispatchFuncIndices[__GLX_DISPATCH_{f.name}]);
        if (_ptr_{f.name} != NULL) {{
""".lstrip("\n")

    text += "            "
    if (func.hasReturn()):
        text += "_ret = "
    text += "(*_ptr_{f.name})({f.callArgs});\n"

    # Handle any added or removed object mappings.
    if (glxFunc.get("add_mapping") != None):
        if (glxFunc["add_mapping"] == "context"):
            text += "            _ret = __glXDispatchAddContextMapping({ex[display]}, _ret, _vendor);\n"
        elif (glxFunc["add_mapping"] == "drawable"):
            text += "            __glXDispatchAddDrawableMapping({ex[display]}, _ret, _vendor);\n"
        elif (glxFunc["add_mapping"] == "config"):
            text += "            _ret = __glXDispatchAddFBConfigMapping({ex[display]}, _ret, _vendor);\n"
        elif (glxFunc["add_mapping"] == "config_list"):
            text += "            _ret = __glXDispatchAddFBConfigListMapping({ex[display]}, _ret, nelements, _vendor);\n"
        else:
            raise ValueError("Unknown add_mapping method: %r" % (glxFunc["add_mapping"],))

    if (glxFunc.get("remove_mapping") != None):
        if (glxFunc["remove_mapping"] == "context"):
            text += "            __glXDispatchApiExports->removeVendorContextMapping({ex[display]}, {ex[remove_key]});\n"
        elif (glxFunc["remove_mapping"] == "drawable"):
            text += "            __glXDispatchApiExports->removeVendorDrawableMapping({ex[display]}, {ex[remove_key]});\n"
        elif (glxFunc["remove_mapping"] == "config"):
            text += "            __glXDispatchApiExports->removeVendorFBConfigMapping({ex[display]}, {ex[remove_key]});\n"
        else:
            raise ValueError("Unknown remove_mapping method: %r" % (glxFunc["add_mapping"],))

    text += "        }}\n"
    text += "    }}\n"
    if (func.hasReturn()):
        text += "    return _ret;\n"
    text += "}}\n\n"

    text = text.format(f=func, ex=glxFunc)
    return text

if (__name__ == "__main__"):
    main()

