/*
 * Copyright (c) 2013, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

// #include "include/scrnintstr.h"

#include <xorg-server.h>
#include <xorgVersion.h>
#include <string.h>
#include <xf86Module.h>
#include <scrnintstr.h>
#include <windowstr.h>
#include <dixstruct.h>
#include <extnsionst.h>
#include <xf86.h>

#include "x11glvnd.h"
#include "x11glvndproto.h"
#include "x11glvndserver.h"
#include "glvnd_list.h"

/*
 * Screen-private structure
 */
typedef struct XGLVScreenPrivRec {
    char *vendorLib;
} XGLVScreenPriv;

DevPrivateKeyRec glvXGLVScreenPrivKey;

/* Dispatch information */
typedef int ProcVectorFunc(ClientPtr);
typedef ProcVectorFunc *ProcVectorFuncPtr;

#define PROC_VECTOR_ENTRY(foo) [X_glv ## foo] = ProcGLV ## foo
#define PROC_PROTO(foo) static int ProcGLV ## foo (ClientPtr client)

PROC_PROTO(QueryXIDScreenMapping);
PROC_PROTO(QueryScreenVendorMapping);

static ProcVectorFuncPtr glvProcVector[X_glvLastRequest] = {
    PROC_VECTOR_ENTRY(QueryXIDScreenMapping),
    PROC_VECTOR_ENTRY(QueryScreenVendorMapping),
};

#undef PROC_VECTOR_ENTRY

static void GLVExtensionInit(void);

/* Module information */
static ExtensionModule glvExtensionModule = {
    GLVExtensionInit,
    XGLV_EXTENSION_NAME,
    NULL,
};

static XF86ModuleVersionInfo x11glvndVersionInfo =
{
    "x11glvnd",
    "NVIDIA Corporation",
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_NUMERIC(4,0,2,0,0),
    1, 0, 0,
    NULL, // ABI_CLASS_EXTENSION,
    ABI_EXTENSION_VERSION,
    MOD_CLASS_EXTENSION,
    {0, 0, 0, 0}
};

static void *glvSetup(void *module, void *opts, int *errmaj, int *errmin);

/*
 * x11glvndModuleData is a magic symbol needed to load the x11glvnd module in
 * the X server.
 */
PUBLIC const XF86ModuleData x11glvndModuleData = { &x11glvndVersionInfo,
                                            glvSetup, NULL };

static void *glvSetup(void *module, void *opts, int *errmaj, int *errmin)
{
    typedef int (*LoaderGetABIVersionProc)(const char *abiclass);
    LoaderGetABIVersionProc pLoaderGetABIVersion;
    int extMajor = 0;

    xf86Msg(X_INFO, "x11glvnd Loading\n");

    if ((pLoaderGetABIVersion = (LoaderGetABIVersionProc)LoaderSymbol("LoaderGetABIVersion"))) {
        extMajor = GET_ABI_MAJOR(pLoaderGetABIVersion(ABI_CLASS_EXTENSION));
    }

    if (extMajor != GET_ABI_MAJOR(ABI_EXTENSION_VERSION)) {
        xf86Msg(X_INFO, "x11glvnd: X server major extension ABI mismatch: expected %d but saw %d\n",
                GET_ABI_MAJOR(ABI_EXTENSION_VERSION), extMajor);
        return NULL;
    }

    LoadExtension(&glvExtensionModule, False);

    return (pointer)1;
}

/*
 * Hook for GLX drivers to register their GLX drawable types.
 */
PUBLIC void _XGLVRegisterGLXDrawableType(RESTYPE rtype)
{
    // TODO
}

enum {
    OPTION_GL_VENDOR,
};



// TODO: make sense to do this instead?
//
//static int ProcGLVQueryXIDVendorMapping(ClientPtr client)
//{
//    // TODO: char *XGLVQueryXIDVendorMapping(XID xid)
//    // Returns the name of the vendor library for this XID
//}

static int ProcGLVQueryXIDScreenMapping(ClientPtr client)
{
   return BadImplementation;
}

static int ProcGLVQueryScreenVendorMapping(ClientPtr client)
{
    return BadImplementation;
}

static int ProcGLVDispatch(ClientPtr client)
{
    REQUEST(xReq);

    if (stuff->data >= X_glvLastRequest) {
        return BadRequest;
    }

    if (!glvProcVector[stuff->data]) {
        return BadImplementation;
    }

    return glvProcVector[stuff->data](client);
}

static int SProcGLVDispatch(ClientPtr client)
{
    return BadImplementation;
}

static void GLVReset(ExtensionEntry *extEntry)
{
    // nop
}

static void GLVExtensionInit(void)
{
    ExtensionEntry *extEntry;
    char ext_name[] = XGLV_EXTENSION_NAME;

    if ((extEntry = AddExtension(ext_name,
                                 XGLV_NUM_EVENTS,
                                 XGLV_NUM_ERRORS,
                                 ProcGLVDispatch,
                                 SProcGLVDispatch,
                                 GLVReset,
                                 StandardMinorOpcode)))
    {
        // do stuff with extEntry?
    }

    // TODO: do the screen -> vendor mappings now
}

