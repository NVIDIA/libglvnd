libglvnd: the GL Vendor-Neutral Dispatch library
================================================

Introduction
------------

This is a work-in-progress implementation of the vendor-neutral dispatch layer
for arbitrating OpenGL API calls between multiple vendors on a per-screen basis,
as described by Andy Ritger's OpenGL ABI proposal [1].

Currently, only the GLX window-system API and OpenGL are supported, but in the
future this library may support EGL and OpenGL ES as well.


Building the library
----------------------

Run `./autogen.sh`. Run `./dbg_configure.sh` to build the library in debug
mode, or `./configure` to build it in release mode. Finally, run `make`.


Code overview
-------------

The code in the src/ directory is organized as follows:

- GLX/ contains code for libGLX, the GLX window-system API library.
- GLdispatch/ contains code for libGLdispatch, which is really just a thin
  wrapper around Mesa's glapi that tries to hide most of the complexity of
  managing dispatch tables. Its interface is defined in GLdispatch.h. This
  implements the guts of the core GL API libraries.
- EGL/ and GLESv{1,2}/ are placeholders for now. GLESv{1,2}/ will be filter
  libraries on libGLdispatch, while EGL/ will contain libEGL, which may be
  implemented similarly to libGLX.
- GL/ and OpenGL/ respectively contain code to generate libGL and libOpenGL,
  which are both merely filter libraries for libGLX and libGLdispatch.
- util/ contains generic utility code, and arch/ contains architecture-specific
  defines.

There are a few good starting points for familiarizing oneself with the code:

- Look at the vendor-library to GLX ABI defined in `libglxabi.h`.
- Follow the flow of `glXGetProcAddress() -> __glDispatchGetProcAddress() ->
  __glapi_get_proc_address()` to see how the dispatch table is updated as new GL
  stubs are generated, and how GLX looks for vendor-library-implemented
  dispatchers for GLX extension functions.
- Follow the flow of `glXMakeContextCurrent() -> __glDispatchMakeCurrent() ->
  _glapi_set_current()` to see how the current dispatch table and state is
  updated by the API library.
- Look at `libglxmapping.c:__glXLookupVendorBy{Name,Screen}()` to see how
  vendor library names are queried. At the same time, look at
  x11glvnd{client,server}.c to see how the "GLVendor" extension which
  retrieves the appropriate mappings is implemented.

The tests/ directory contains several unit tests which verify that dispatching
to different vendors actually works. Run `make check` to run these unit tests.
Note some of the unit tests require a special X server configuration and
are skipped by default.  To include these tests (and X server
initialization/teardown), run `make check DO_X11_TESTS=1`.

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
          │     ┌─────▾─────┐             │                    ┌──────────────┐
          │     │           │             │                    │              │
          │     │ libOpenGL │             │                    │              │
          │     │           │             │                    │  X server    │
          │     └─────┬─────┘             │                    │              │
          │        DT_FILTER              │                    │              │
          │     ┌─────▾──────────┐ ┌──────▾────────┐           │ ┌──────────┐ │
          │     │                │ │               │           └─│GLVendor  │─┘
          │     │ [mapi/glapi]   ◂─▸               │             │extension │
          │     │ libGLdispatch  │ │   libGLX      ├─────────────▸──────────┘
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
* `A ── DT_FILTER ──▸ B` indicates that DSO A is a filter library on DSO B, and
  symbols exported by A are resolved via ELF symbol filtering to entrypoints in
  B.

libGLX manages loading GLX vendor libraries and dispatching GLX core and
extension functions to the right vendor.

GLVendor is a simple X extension which allows libGLX to determine the number of
the screen belonging to an arbitrary drawable XID, and also the GL vendor to use
for a given screen.

libGLdispatch implements core GL dispatching and TLS. It acts as a thin wrapper
around glapi which provides some higher-level functionality for managing
dispatch tables, requesting vendor proc addresses, and making current to a given
context + dispatch table. This is a separate library rather than statically
linked into libGLX, since current dispatch tables will eventually be shared
between GLX and EGL, similarly to how glapi operates when Mesa is compiled with
the --shared-glapi option.

libOpenGL is a filter library on libGLdispatch which exposes OpenGL 4.x core and
compatibility entry points. Eventually, there will be a libGLESv{1,2} which will
also be filter libraries on libGLdispatch that expose GLES entry points.

libGL is a filter library on libGLdispatch and libGLX which is provided for
backwards-compatibility with applications which link against the old ABI.

NOTE: Logically, libGL should be a filter library on libOpenGL rather than
libGLdispatch, as libGLdispatch is an implementation detail.  The current
arrangement is in place to work around a loader bug (present on at least ld.so
v.2.12) where symbols in libGL don't properly resolve to libGLdispatch symbols
if it filters libOpenGL instead of libGLdispatch.

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

  * Load the correspending `libGLX_VENDOR.so`.

  * Read the vendor's GLX dispatch table from the `libGLX_VENDOR.so`.

  * Cache that Display + screen <=> vendor dispatch table mapping, for
    use in subsequent dispatching.

* Some GLX entry points imply an X screen by a GLX object they
  specify.  Such GLX objects are:

    GLXContext  (an opaque pointer)
    GLXFBConfig (an opaque pointer)
    GLXPixmap   (an XID)
    GLXDrawable (an XID)
    GLXWindow   (an XID)
    GLXPbuffer  (an XID)

  To map from object to screen, record the corresponding screen when
  the object is created.  This means the current process needs to see
  a GLX call to create the object.  In the case of the opaque
  pointers, this is reasonable, since the pointer is only meaningful
  within the current process.  But XIDs could be created by another
  process.  See the Issues section below.

* To minimize code complexity from error checking, define a noop GLX
  dispatch table.  This is returned by `__glXGet{,Current}Dispatch()` in
  case no other dispatch table can be found.

* Similarly, `__glXScreenFrom{Context,FBConfig,Drawable}()` may fail to
  find a screen matching the specified GLX object.  In this case, the
  returned screen number is -1, but the caller should just pass the
  screen number through to `__glXGetDispatch()` or
  `__glX{Add,Remove}Screen{Context,FBConfig,Drawable}Mapping()`.  Those
  functions are expected to deal gracefully with the invalid screen
  number.

Issues
------

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

* Should we map XIDs directly to vendors, rather than to screens?

* The current libGLX implementation stores the mapping between screen and all
  objects of the same type in one hash table.  I.e., all pointer types
  (GLXContext and GLXFBConfig) in one table, and all XID types (GLXDrawable,
  GLXPixmap, GLXWindow, and GLXPbuffer) in another table.  Should there instead
  be more finer-grained hash tables?  There probably couldn't be finer-grained
  tables for XIDs, because GLXDrawable is used interchangably with the other
  XID-based types.

* The issue above applies to XIDs in the x11glvnd extension as well: we
  currently don't make any distinction between window, GLX pixmap, GLX window, or
  GLX pbuffer XIDs.

* Querying an XID <=> screen mapping without somehow "locking" the XID is
  inherently racy, since a different process may destroy the drawable, and X
  may recycle the XID, after the mapping is saved client-side. Is there a mechanism
  we could use to notify the API library when a mapping is no longer valid?

* Currently the library does not attempt to clean up allocations and
  unload vendor libraries if the application unloads it. This will need to be
  implemented eventually for the library to be usable in a production
  environment. What will the sequencing of this look like? Should we also hook
  into XCloseDisplay()?

* Should x11glvnd be an extension on top of GLX 1.4, or a "GLX Next" feature?

TODO
----

* Refactor so the core OpenGL dispatch table, and the vendor-library ABI exposed
  to manipulate this table, is independent of the libGLX ABI.

* Fix glXGetClientString() to take a real "union" of strings, rather than just
  concatenating them.

* Implement libEGL, and libGLESv{1,2}.

* Root-cause and fix the loader bug that prevents libGL from filtering libOpenGL
  rather than libGLdispatch to pick up core GL symbols.

* Currently, running "ldd libGL.so" fails with the following assertion (for ldd
  v.2.15 on Ubuntu 12.04 LTS):

    Inconsistency detected by ld.so: dl-deps.c: 592: _dl_map_object_deps:
    Assertion `map->l_searchlist.r_list[0] == map' failed!

  Root-cause and fix this bug.

References
----------

[1] https://github.com/aritger/linux-opengl-abi-proposal/blob/master/linux-opengl-abi-proposal.txt

Acknowledgements
-------

Thanks to Andy Ritger for the original libGLX implementation and README
documentation.

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

