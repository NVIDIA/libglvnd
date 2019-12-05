libglvnd: the GL Vendor-Neutral Dispatch library
================================================

Introduction
------------

libglvnd is a vendor-neutral dispatch layer for arbitrating OpenGL API calls
between multiple vendors. It allows multiple drivers from different vendors to
coexist on the same filesystem, and determines which vendor to dispatch each
API call to at runtime.

Both GLX and EGL are supported, in any combination with OpenGL and OpenGL ES.

libglvnd was originally described in Andy Ritger's OpenGL ABI proposal [1].

The official repository for libglvnd is hosted on FreeDesktop.org's GitLab:
https://gitlab.freedesktop.org/glvnd/libglvnd


Building the library
----------------------

libglvnd build-depends on libx11, glproto and libxext.
On Debian and derivatives, run:

    sudo apt-get install libxext-dev libx11-dev x11proto-gl-dev

Run `./autogen.sh`, then run `./configure` and `make`.

Alternatively you can use meson and ninja, which is much faster but should be
otherwise the same (You will need packages from above too):

    sudo apt-get install ninja-build meson
    meson builddir 
    ninja -C builddir

Meson 0.48.0 is currently required, if your distro doesn't have meson 0.48 in
the repos you can try the methods suggested
[here](https://mesonbuild.com/Getting-meson.html).

Code overview
-------------

The code in the src/ directory is organized as follows:

- GLX/ contains code for libGLX, the GLX window-system API library.
- EGL/ contains code for libEGL, the EGL window-system API library.
- GLdispatch/ contains code for libGLdispatch, which is responsible for
  dispatching OpenGL functions to the correct vendor library based on the
  current EGL or GLX rendering context. This implements the guts of the GL API
  libraries. Most of the dispatch code is based on Mesa's glapi.
- OpenGL/, GLESv1/, and GLESv2/ contain code to generate libOpenGL.so,
  libGLESv1\_CM.so, and libGLESv2.so, respectively. All three are merely
  wrapper libraries for libGLdispatch. Ideally, these could be implemented via
  ELF symbol filtering, but in practice they need to be implemented manually.
  See the Issues section for details on why this is the case.
- GL/ contains code for libGL. This is a wrapper around libGLdispatch and
  libGLX.
- util/ contains generic utility code.

In addition, libglvnd uses a GLX extension,
[GLX\_EXT\_libglvnd](https://khronos.org/registry/OpenGL/extensions/EXT/GLX_EXT_libglvnd.txt),
to determine which vendor library to use for a screen or XID.

There are a few good starting points for familiarizing oneself with the code:

- Look at the vendor-library to GLX ABI defined in `libglxabi.h`.
- Follow the flow of `glXGetProcAddress() -> __glDispatchGetProcAddress() ->
  _glapi_get_proc_address()` to see how the dispatch table is updated as new GL
  stubs are generated, and how GLX looks for vendor-library-implemented
  dispatchers for GLX extension functions.
- Follow the flow of `glXMakeContextCurrent() -> __glDispatchMakeCurrent() ->
  _glapi_set_current()` to see how the current dispatch table and state is
  updated by the API library.
- Look at `libglxmapping.c:__glXLookupVendorBy{Name,Screen}()` to see how
  vendor library names are queried.
- For EGL, follow the flow of `eglGetPlatformDisplay()` to see how EGL selects
  a vendor library.

The tests/ directory contains several unit tests which verify that dispatching
to different vendors actually works. Run `make check` to run these unit tests.

Architecture
------------

The library organization differs slightly from that of Andy's original proposal.
See the diagram below:

<pre>
                ┌──────────────────────────────────┐
                │                                  │
          ┌─────┤        Application               │
          │     │                                  │
          │     └─────┬───────────────────┬────────┘
          │           │                   │
          │     ┌─────▾─────┐             │                    ┌──────────────────────┐
          │     │           │             │                    │                      │
          │     │ libOpenGL │             │                    │                      │
          │     │           │             │                    │  X server            │
          │     └─────┬─────┘             │                    │                      │
          │        DT_FILTER              │                    │                      │
          │     ┌─────▾──────────┐ ┌──────▾────────┐           │ ┌──────────────────┐ │
          │     │                │ │               │           └─│GLX_EXT_libglvnd  │─┘
          │     │ [mapi/glapi]   ◂─▸               │             │extension         │
          │     │ libGLdispatch  │ │   libGLX      ├─────────────▸──────────────────┘
          │     │                │ │               ◂──────────┬─────────────────┐
          │     └───────▴────────┘ └──────▴────────┘          │                 │
          │         DT_FILTER         DT_FILTER             ┌─▾─────────┐   ┌───▾────────┐
          │     ┌───────┴─────────────────┴────────┐        │           │   │            │
          │     │                                  │        │           │   │            │
          └─────▸           libGL                  │        │ GLX_vendor│   │ GLX_vendor2│
                └──────────────────────────────────┘        │           │   │            │
                                                            └───────────┘   └────────────┘
</pre>

In this diagram,

* `A ───▸ B` indicates that module A calls into module B.
* `A ── DT_FILTER ──▸ B` indicates that DSO A is (logically) a filter library on
  DSO B.  If ELF symbol filtering is enabled, symbols exported by A are resolved
  to entrypoints in B.

libGLX manages loading GLX vendor libraries and dispatching GLX core and
extension functions to the right vendor.

GLX\_EXT\_libglvnd is a simple GLX extension which allows libGLX to determine
the number of the screen belonging to an arbitrary drawable XID, and also the
GL vendor to use for a given screen.

libGLdispatch implements core GL dispatching and TLS. It acts as a thin wrapper
around glapi which provides some higher-level functionality for managing
dispatch tables, requesting vendor proc addresses, and making current to a given
context + dispatch table. This is a separate library rather than statically
linked into libGLX, since the same dispatching code is used by both GLX and
EGL.

libOpenGL is a wrapper library to libGLdispatch which exposes OpenGL 4.5 core and
compatibility entry points.

libGLESv{1,2} are wrapper libraries to libGLdispatch which expose OpenGL ES
entrypoints.

libGL is a wrapper library to libGLdispatch and libGLX which is provided for
backwards-compatibility with applications which link against the old ABI.

Note that since all OpenGL functions are dispatched through the same table in
libGLdispatch, it doesn't matter which library is used to find the entrypoint.
The same OpenGL function in libGL, libOpenGL, libGLES, and the function pointer
returned by glXGetProcAddress are all interchangeable.

### OpenGL dispatching ###

By definition, all OpenGL functions are dispatched based on the current
context. OpenGL dispatching is handled in libGLdispatch, which is used by both
EGL and GLX.

libGLdispatch uses a per-thread dispatch table to look up the correct vendor
library function for every OpenGL function.

When an application calls eglMakeCurrent or glXMakeCurrent, the EGL or GLX
library finds the correct dispatch table and then calls into libGLdispatch to
set that table for the current thread.

Since they're all dispatched through the common libGLdispatch layer, that also
means that all OpenGL entrypoints will work correctly, regardless of whether
the current context is from EGL or GLX.

### GLX dispatching ###

Unlike core OpenGL functions, whose vendor can be determined from the current
context, many GLX functions are context-independent. In order to successfully
map GLX API calls to the right vendor, we use the following strategy:

* Most GLX entry points specify (either explicitly, or implicitly) an
  X screen.

* On a per-entry point basis, dispatch the call to the
  `libGLX_VENDOR.so` for that screen.

* The first time `libGLX.so` gets called with a unique combination of X
  Display + screen, do the following:

  * Use the Display connection to query the X server for the GLX
    vendor of that X screen.

  * Load the corresponding `libGLX_VENDOR.so`.

  * Read the vendor's GLX dispatch table from the `libGLX_VENDOR.so`.

  * Cache that Display + screen <=> vendor dispatch table mapping, for
    use in subsequent dispatching.

* Some GLX entry points imply an X screen by a GLX object they
  specify. Such GLX objects are:

  * GLXContext  (an opaque pointer)
  * GLXFBConfig (an opaque pointer)
  * GLXPixmap   (an XID)
  * GLXDrawable (an XID)
  * GLXWindow   (an XID)
  * GLXPbuffer  (an XID)

  To map from object to screen, record the corresponding screen when
  the object is created. This means the current process needs to see
  a GLX call to create the object. In the case of the opaque
  pointers, this is reasonable, since the pointer is only meaningful
  within the current process.

  XIDs, however, can be created by another process, so libGLX may not know in
  advance which screen they belong to. To deal with that, libGLX queries the
  server using the GLX extension GLX\_EXT\_libglvnd.

### EGL dispatching ###

EGL dispatching works similarly to GLX, but there are fewer object types to
deal with. Almost all EGL functions are dispatched based on an EGLDisplay or
EGLDeviceEXT parameter.

EGL can't rely on asking an X server for a vendor name like GLX can, so
instead, it enumerates and loads every available vendor library. Loading every
vendor is also needed to support extensions such as
EGL\_EXT\_device\_enumeration.

In order to find the available vendor libraries, each vendor provides a JSON
file in a well-known directory, similar to how Vulkan ICD's are loaded.
Please see [EGL ICD enumeration](src/EGL/icd_enumeration.md) for more details.

When the application calls eglGetPlatformDisplay, EGL will simply call into
each vendor library until it finds one that succeeds. After that, whichever
vendor succeeded owns that display.

As with GLX, vendor libraries must provide dispatch stubs for any display or
device extensions that they support, so that they can add new extensions
without having to modify libglvnd.

Since libglvnd passes eglGetPlatformDisplay calls through to each vendor, a
vendor can also add a new platform extension (e.g., EGL\_KHR\_platform\_x11)
without changing libglvnd.

Other EGL client extensions, by definition, do require modifying libglvnd.
Those are handled on a case-by-case basis.

Issues
------

* Ideally, several components of libglvnd (namely, the `libGL` wrapper library
  and the `libOpenGL, libGLES{v1_CM,v2}` interface libraries) could be
  implemented via ELF symbol filtering (see [2] for a demonstration of this).
  However, a loader bug (tracked in [3]) makes this mechanism unreliable:
  dlopen(3)ing a shared library with `DT_FILTER` fields can crash the
  application.  Instead, for now, ELF symbol filtering is disabled by default,
  and an alternate approach is used to implement these libraries.

* The library currently indirectly associates a drawable with a vendor,
  by first mapping a drawable to its screen, then mapping the screen to its
  vendor. However, it may make sense in render offload scenarios to allow direct
  mapping from drawables to vendors, so multiple vendors could potentially
  operate on drawables in the same screen. The problem with this is that several
  GLX functions, such as glXChooseFBConfig(), explicitly refer to screens, and
  so it becomes a gray area which vendor the call should be dispatched to. Given
  this issue, does it still make more sense to use a direct drawable to vendor
  mapping? How would this be implemented? Should we add new API calls to "GLX
  Next"?

  * Note that the (drawable -> screen -> vendor) mapping mainly exists in the
    GLX_EXT_libglvnd extension. libGLX itself keeps a simple
    (drawable -> vendor) mapping, and exposes that mapping to the vendor
    libraries.

* Along the same lines, would it be useful to include a
  "glXGetProcAddressFromVendor()" or "glXGetProcAddressFromScreen()" entrypoint
  in a new GLX version to obviate the need for this library in future
  applications?

* Global state is required by both libGLX.so and libGLdispatch.so for various
  purposes, and needs to be protected by locks in multithreaded environments.
  Is it reasonable for the vendor-neutral library to depend on pthreads for
  implementing these locks?

  While there is no harm in having the API libraries link against pthreads even
  if the application does not, we would like to avoid pthread locking overhead
  if the application is single-threaded.  Hence, this library uses a
  `glvnd_pthread` wrapper library which provides single-threaded fallbacks for
  applications which are not linked against pthreads.  It is expected that
  multi-threaded applications will either statically link against pthreads, or
  load pthreads prior to loading libGL.

* Is using a hash table to store GLX extension entrypoints performant enough for
  dispatching? Should we be using a flat array instead?

* How should malloc(3) failures be handled?

* How should forking be handled?

* The current libGLX implementation stores all GLXContext and GLXFBConfig
  handles in global hashtables, which means that GLXContext and GLXFBConfig
  handles must be unique between vendors. That is, two vendor libraries must
  not come up with the same handle value for a GLXContext or GLXFBConfig. To
  that end, GLXContext and GLXFBConfig handles must be pointers to memory
  addresses that the vendor library somehow controls. The values are otherwise
  opaque.

* Querying an XID <=> screen mapping without somehow "locking" the XID is
  inherently racy, since a different process may destroy the drawable, and X
  may recycle the XID, after the mapping is saved client-side. Is there a mechanism
  we could use to notify the API library when a mapping is no longer valid?

References
----------

[1] https://github.com/aritger/linux-opengl-abi-proposal/blob/master/linux-opengl-abi-proposal.txt

[2] https://github.com/aritger/libgl-elf-tricks-demo

[3] https://sourceware.org/bugzilla/show_bug.cgi?id=16272

Acknowledgements
-------

Thanks to Andy Ritger for the original libGLX implementation and README
documentation.

### libglvnd ###

libglvnd itself (excluding components listed below) is licensed as follows:

    Copyright (c) 2013, NVIDIA CORPORATION.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and/or associated documentation files (the
    "Materials"), to deal in the Materials without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Materials, and to
    permit persons to whom the Materials are furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    unaltered in all copies or substantial portions of the Materials.
    Any additions, deletions, or changes to the original source files
    must be clearly indicated in accompanying documentation.

    If only executable code is distributed, then the accompanying
    documentation must state that "this software is based in part on the
    work of the Khronos Group."

    THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

### X.Org ###

libglvnd contains list.h, a linked list implementation from the X.Org project.
Source code from the X.Org project is available from:

    http://cgit.freedesktop.org/xorg/xserver

list.h carries the following license:

    Copyright © 2010 Intel Corporation
    Copyright © 2010 Francisco Jerez <currojerez@riseup.net>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice (including the next
    paragraph) shall be included in all copies or substantial portions of the
    Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.

### Mesa ###

libglvnd contains code from the Mesa project. Source code from the Mesa project
is available from:

    http://cgit.freedesktop.org/mesa/mesa

The default Mesa license is as follows:

    Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
    BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### uthash ###

libglvnd uses the hash table implementation 'uthash':

    http://troydhanson.github.io/uthash/

This library carries the following copyright notice:

    Copyright (c) 2005-2013, Troy D. Hanson
    http://troydhanson.github.com/uthash/
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
    OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### buildconf ###

libglvnd uses the buildconf autotools bootstrapping script 'autogen.sh':

    http://freecode.com/projects/buildconf

This script carries the following copyright notice:

    Copyright (c) 2005-2009 United States Government as represented by
    the U.S. Army Research Laboratory.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

    3. The name of the author may not be used to endorse or promote
    products derived from this software without specific prior written
    permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
    OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### ax-pthread ###

libglvnd uses the `AX_PTHREAD` autoconf macro for detecting pthreads.
The implementation of this macro carries the following license:

    Copyright (c) 2008 Steven G. Johnson <stevenj@alum.mit.edu>
    Copyright (c) 2011 Daniel Richard G. <skunk@iSKUNK.ORG>

    This program is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the 
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but 
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
    Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program. If not, see <http://www.gnu.org/licenses/>.

    As a special exception, the respective Autoconf Macro's copyright owner
    gives unlimited permission to copy, distribute and modify the configure
    scripts that are the output of Autoconf when processing the Macro. You 
    need not follow the terms of the GNU General Public License when using
    or distributing such scripts, even though portions of the text of the 
    Macro appear in them. The GNU General Public License (GPL) does govern
    all other use of the material that constitutes the Autoconf Macro.

    This special exception to the GPL applies to versions of the Autoconf
    Macro released by the Autoconf Archive. When you make and distribute a
    modified version of the Autoconf Macro, you may extend this special
    exception to the GPL to apply to your modified version as well.

libglvnd uses the cJSON library for reading JSON files:

https://github.com/DaveGamble/cJSON

This library carries the following copyright notice:

    Copyright (c) 2009 Dave Gamble

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

