########################################################################
# Copyright (c) 2013, NVIDIA CORPORATION.
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
########################################################################

########################################################################
#
# glx_funcs.spec: parsed by gen_stubs.pl
#
# Spec file used to generate no-op GLX functions and libGL wrapper stubs.
#
# Each function prototype uses the following format:
#
# function "FunctionName"
#     returns "return_type" "default_return_value"
#     param   "param_type1" "param_name1"
#     param   "param_type2" "param_name2"
#     ...
#
# Defines a spec for function fooBar.
#
# The "returns" line defines the return type of the function. If the return type
# is void, the default_return_value field is ignored; otherwise, it is used to
# determine the return value if the dispatcher fails to redirect the function
# call to a vendor.
#
# Each "param" line defines a different argument in the function prototype.
#
# A "glx14ep" line indicates this function should be plugged into the static
# dispatch table.
#
# A (rough) BNF syntax for this spec file follows:
#
# <spec-file> :== <function-description>*
# <function-description> :== <function-line> <prop-lines> <end-of-function-description>
# <function-line> := "function" <blank> <text> <end-of-line>
# <prop-lines> :== ( <param-line> |  <returns-line> | "glx14ep" )*
# <param-line> :== "param" <text> <text> <end-of-line>
# <returns-line> :== "returns" <text> <text> <end-of-line>
# <end-of-function-description> :== <blank-line>+ | <end-of-file>
#
########################################################################

function             "ChooseVisual"
    returns          "XVisualInfo*"       "NULL"
    param            "Display *"          "dpy"
    param            "int"                "screen"
    param            "int *"              "attrib_list"
    glx14ep

function             "CopyContext"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXContext"         "src"
    param            "GLXContext"         "dst"
    param            "unsigned long"      "mask"
    glx14ep

function             "CreateContext"
    returns          "GLXContext"         "None"
    param            "Display *"          "dpy"
    param            "XVisualInfo *"      "vis"
    param            "GLXContext"         "share_list"
    param            "Bool"               "direct"
    glx14ep


function             "CreateGLXPixmap"
    returns          "GLXPixmap"          "None"
    param            "Display *"          "dpy"
    param            "XVisualInfo *"      "vis"
    param            "Pixmap"             "pixmap"
    glx14ep


function             "DestroyContext"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXContext"         "ctx"
    glx14ep


function             "DestroyGLXPixmap"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXPixmap"          "pix"
    glx14ep


function             "GetConfig"
    returns          "int"                "GLX_BAD_SCREEN"
    param            "Display *"          "dpy"
    param            "XVisualInfo *"      "vis"
    param            "int"                "attrib"
    param            "int *"              "value"
    glx14ep


function             "IsDirect"
    returns          "Bool"               "False"
    param            "Display *"          "dpy"
    param            "GLXContext"         "ctx"
    glx14ep


function             "MakeCurrent"
    returns          "Bool"               "False"
    param            "Display *"          "dpy"
    param            "GLXDrawable"        "drawable"
    param            "GLXContext"         "ctx"
    glx14ep


function             "SwapBuffers"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXDrawable"        "drawable"
    glx14ep


function             "UseXFont"
    returns          "void"               ""
    param            "Font"               "font"
    param            "int"                "first"
    param            "int"                "count"
    param            "int"                "list_base"
    glx14ep


function             "WaitGL"
    returns          "void"               ""
    glx14ep


function             "WaitX"
    returns          "void"               ""
    glx14ep


function             "QueryServerString"
    returns          "const char *"       "NULL"
    param            "Display *"          "dpy"
    param            "int"                "screen"
    param            "int"                "name"
    glx14ep


function             "GetClientString"
    returns          "const char *"       "NULL"
    param            "Display *"          "dpy"
    param            "int"                "name"
    glx14ep


function             "QueryExtensionsString"
    returns          "const char *"       "NULL"
    param            "Display *"          "dpy"
    param            "int"                "screen"
    glx14ep


function             "ChooseFBConfig"
    returns          "GLXFBConfig *"      "NULL"
    param            "Display *"          "dpy"
    param            "int"                "screen"
    param            "const int *"        "attrib_list"
    param            "int *"              "nelements"
    glx14ep


function             "CreateNewContext"
    returns          "GLXContext"         "None"
    param            "Display *"          "dpy"
    param            "GLXFBConfig"        "config"
    param            "int"                "render_type"
    param            "GLXContext"         "share_list"
    param            "Bool"               "direct"
    glx14ep


function             "CreatePbuffer"
    returns          "GLXPbuffer"         "None"
    param            "Display *"          "dpy"
    param            "GLXFBConfig"        "config"
    param            "const int *"        "attrib_list"
    glx14ep


function             "CreatePixmap"
    returns          "GLXPixmap"          "None"
    param            "Display *"          "dpy"
    param            "GLXFBConfig"        "config"
    param            "Pixmap"             "pixmap"
    param            "const int *"        "attrib_list"
    glx14ep


function             "CreateWindow"
    returns          "GLXWindow"          "None"
    param            "Display *"          "dpy"
    param            "GLXFBConfig"        "config"
    param            "Window"             "win"
    param            "const int *"        "attrib_list"
    glx14ep


function             "DestroyPbuffer"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXPbuffer"         "pbuf"
    glx14ep


function             "DestroyPixmap"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXPixmap"          "pixmap"
    glx14ep


function             "DestroyWindow"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXWindow"          "win"
    glx14ep


function             "GetFBConfigAttrib"
    returns          "int"                "0"
    param            "Display *"          "dpy"
    param            "GLXFBConfig"        "config"
    param            "int"                "attribute"
    param            "int *"              "value"
    glx14ep


function             "GetFBConfigs"
    returns          "GLXFBConfig *"      "NULL"
    param            "Display *"          "dpy"
    param            "int"                "screen"
    param            "int *"              "nelements"
    glx14ep


function             "GetSelectedEvent"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXDrawable"        "draw"
    param            "unsigned long *"    "event_mask"
    glx14ep


function             "GetVisualFromFBConfig"
    returns          "XVisualInfo *"      "NULL"
    param            "Display *"          "dpy"
    param            "GLXFBConfig"        "config"
    glx14ep


function             "MakeContextCurrent"
    returns          "Bool"               "False"
    param            "Display *"          "display"
    param            "GLXDrawable"        "draw"
    param            "GLXDrawable"        "read"
    param            "GLXContext"         "ctx"
    glx14ep


function             "QueryContext"
    returns          "int"                "0"
    param            "Display *"          "dpy"
    param            "GLXContext"         "ctx"
    param            "int"                "attribute"
    param            "int *"              "value"
    glx14ep


function             "QueryDrawable"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXDrawable"        "draw"
    param            "int"                "attribute"
    param            "unsigned int *"     "value"
    glx14ep


function             "SelectEvent"
    returns          "void"               ""
    param            "Display *"          "dpy"
    param            "GLXDrawable"        "draw"
    param            "unsigned long"      "event_mask"
    glx14ep


function             "GetCurrentContext"
    returns          "GLXContext"         "None"


function             "GetCurrentDrawable"
    returns          "GLXDrawable"        "None"


function             "GetCurrentReadDrawable"
    returns          "GLXDrawable"        "None"


function             "GetProcAddress"
    returns          "__GLXextFuncPtr"    "NULL"
    param            "const GLubyte *"    "procName"


function             "GetProcAddressARB"
    returns          "__GLXextFuncPtr"    "NULL"
    param            "const GLubyte *"    "procName"


function             "QueryExtension"
    returns          "Bool"               "False"
    param            "Display *"          "dpy"
    param            "int *"              "error_base"
    param            "int *"              "event_base"


function             "QueryVersion"
    returns          "Bool"               "False"
    param            "Display *"          "dpy"
    param            "int *"              "major"
    param            "int *"              "minor"

