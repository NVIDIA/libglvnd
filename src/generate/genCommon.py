#!/usr/bin/env python

# (C) Copyright 2015, NVIDIA CORPORATION.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Authors:
#    Kyle Brenneman <kbrenneman@nvidia.com>

import sys
import collections
import re
import xml.etree.cElementTree as etree

MAPI_TABLE_NUM_DYNAMIC = 4096

def getFunctions(xmlFile):
    """
    Reads an XML file and returns all of the functions defined in it.

    xmlFile should be the path to Khronos's gl.xml file. The return value is a
    sequence of FunctionDesc objects, ordered by slot number.
    """
    root = etree.parse(xmlFile).getroot()
    return getFunctionsFromRoot(root)

def getFunctionsFromRoot(root):
    functions = _getFunctionList(root)

    # Sort the function list by name.
    functions = sorted(functions, key=lambda f: f.name)

    # Assign a slot number to each function. This isn't strictly necessary,
    # since you can just look at the index in the list, but it makes it easier
    # to include the slot when formatting output.
    for i in xrange(len(functions)):
        functions[i] = functions[i]._replace(slot=i)

    return functions

class FunctionArg(collections.namedtuple("FunctionArg", "type name")):
    @property
    def dec(self):
        """
        Returns a "TYPE NAME" string, suitable for a function prototype.
        """
        rv = str(self.type)
        if(not rv.endswith("*")):
            rv += " "
        rv += self.name
        return rv

class FunctionDesc(collections.namedtuple("FunctionDesc", "name rt args slot")):
    def hasReturn(self):
        """
        Returns true if the function returns a value.
        """
        return (self.rt != "void")

    @property
    def decArgs(self):
        """
        Returns a string with the types and names of the arguments, as you
        would use in a function declaration.
        """
        if(len(self.args) == 0):
            return "void"
        else:
            return ", ".join(arg.dec for arg in self.args)

    @property
    def callArgs(self):
        """
        Returns a string with the names of the arguments, as you would use in a
        function call.
        """
        return ", ".join(arg.name for arg in self.args)

    @property
    def basename(self):
        assert(self.name.startswith("gl"))
        return self.name[2:]

def _getFunctionList(root):
    for elem in root.findall("commands/command"):
        yield _parseCommandElem(elem)

def _parseCommandElem(elem):
    protoElem = elem.find("proto")
    (rt, name) = _parseProtoElem(protoElem)

    args = []
    for ch in elem.findall("param"):
        # <param> tags have the same format as a <proto> tag.
        args.append(FunctionArg(*_parseProtoElem(ch)))
    func = FunctionDesc(name, rt, tuple(args), slot=None)

    return func

def _parseProtoElem(elem):
    # If I just remove the tags and string the text together, I'll get valid C code.
    text = _flattenText(elem)
    text = text.strip()
    m = re.match(r"^(.+)\b(\w+)(?:\s*\[\s*(\d*)\s*\])?$", text, re.S)
    if (m):
        typename = _fixupTypeName(m.group(1))
        name = m.group(2)
        if (m.group(3)):
            # HACK: glPathGlyphIndexRangeNV defines an argument like this:
            # GLuint baseAndCount[2]
            # Convert it to a pointer and hope for the best.
            typename += "*"
        return (typename, name)
    else:
        raise ValueError("Can't parse element %r -> %r" % (elem, text))

def _flattenText(elem):
    """
    Returns the text in an element and all child elements, with the tags
    removed.
    """
    text = ""
    if(elem.text != None):
        text = elem.text
    for ch in elem:
        text += _flattenText(ch)
        if(ch.tail != None):
            text += ch.tail
    return text

def _fixupTypeName(typeName):
    """
    Converts a typename into a more consistant format.
    """

    rv = typeName.strip()

    # Replace "GLvoid" with just plain "void".
    rv = re.sub(r"\bGLvoid\b", "void", rv)

    # Remove the vendor suffixes from types that have a suffix-less version.
    rv = re.sub(r"\b(GLhalf|GLintptr|GLsizeiptr|GLint64|GLuint64)(?:ARB|EXT|NV|ATI)\b", r"\1", rv)

    rv = re.sub(r"\bGLvoid\b", "void", rv)

    # Clear out any leading and trailing whitespace.
    rv = rv.strip()

    # Remove any whitespace before a '*'
    rv = re.sub(r"\s+\*", r"*", rv)

    # Change "foo*" to "foo *"
    rv = re.sub(r"([^\*])\*", r"\1 *", rv)

    # Condense all whitespace into a single space.
    rv = re.sub(r"\s+", " ", rv)

    return rv

