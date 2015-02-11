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
#include <ctype.h>

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

#define XGLV_ABI_HAS_LOAD_EXTENSION_LIST \
    (GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) >= 17)
#define XGLV_SET_SCREEN_PRIVATE(pScreen, priv) \
    dixSetPrivate(&(pScreen)->devPrivates, &glvXGLVScreenPrivKey, priv)
#define XGLV_SCREEN_PRIVATE(pScreen) \
    ((XGLVScreenPriv *)dixLookupPrivate(&(pScreen)->devPrivates, &glvXGLVScreenPrivKey));
#define XGLV_INIT_PRIVATE_SPACE() \
    dixRegisterPrivateKey(&glvXGLVScreenPrivKey, PRIVATE_SCREEN, 0)

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
#ifdef XVENDORNAME
    XVENDORNAME,
#else
    "NVIDIA Corporation",
#endif
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

#if XGLV_ABI_HAS_LOAD_EXTENSION_LIST
    LoadExtensionList(&glvExtensionModule, 1, False);
#else
    LoadExtension(&glvExtensionModule, False);
#endif

    return (pointer)1;
}

typedef struct DrawableTypeRec {
    RESTYPE rtype;
    struct glvnd_list entry;
} XGLVDrawableType;

struct glvnd_list xglvDrawableTypes;

int LookupXIDScreenMapping(ClientPtr client, XID xid)
{
    DrawablePtr pDraw;
    Status status;
    XGLVDrawableType *drawType;

    glvnd_list_for_each_entry(drawType, &xglvDrawableTypes, entry) {
        pDraw = NULL;
        status = dixLookupResourceByType((void **)&pDraw, xid,
                                        RT_WINDOW, client,
                                        BadDrawable);
        if (status == Success) {
            break;
        }
    }

    if (pDraw) {
        return pDraw->pScreen->myNum;
    } else {
        return -1;
    }
}

/*
 * Hook for GLX drivers to register their GLX drawable types.
 */
PUBLIC void _XGLVRegisterGLXDrawableType(RESTYPE rtype)
{
    XGLVDrawableType *drawType = malloc(sizeof(*drawType));

    drawType->rtype = rtype;
    glvnd_list_add(&drawType->entry, &xglvDrawableTypes);
}

enum {
    OPTION_GL_VENDOR,
};

static char *GetVendorForThisScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrnInfo = xf86Screens[pScreen->myNum];
    const char *str;
    char *processedStr;
    OptionInfoRec options[2];

    options[0].token = OPTION_GL_VENDOR;
    options[0].name = XGLV_X_CONFIG_OPTION_NAME;
    options[0].type = OPTV_STRING;
    memset(&options[0].value, 0, sizeof(options[0].value));
    options[0].found = False;

    /* Fill a blank entry to the table */
    options[1].token = -1;
    options[1].name = NULL;
    options[1].type = OPTV_NONE;
    memset(&options[1].value, 0, sizeof(options[1].value));
    options[1].found = False;

    if (!pScrnInfo->options) {
        xf86CollectOptions(pScrnInfo, NULL);
    }

    xf86ProcessOptions(pScreen->myNum,
                       pScrnInfo->options,
                       options);

    str = xf86GetOptValString(options, OPTION_GL_VENDOR);
    if (!str) {
        // Fall back to the driver name if no explicit option specified
        str = pScrnInfo->name;
    }
    if (!str) {
        str = "unknown";
    }

    processedStr = strdup(str);
    if (processedStr) {
        size_t i;
        size_t len = strlen(processedStr);
        for (i = 0; i < len; i++) {
            processedStr[i] = tolower(processedStr[i]);
        }
    }

    return processedStr;
}

static Bool xglvScreenInit(ScreenPtr pScreen)
{
    XGLVScreenPriv *pScreenPriv;

    pScreenPriv = malloc(sizeof(XGLVScreenPriv));

    if (!pScreenPriv) {
        return False;
    }

    // Get the vendor library for this screen
    pScreenPriv->vendorLib = GetVendorForThisScreen(pScreen);

    if (!pScreenPriv->vendorLib) {
        free(pScreenPriv);
        return False;
    }

    XGLV_SET_SCREEN_PRIVATE(pScreen, pScreenPriv);

    return True;
}

// TODO: make sense to do this instead?
//
//static int ProcGLVQueryXIDVendorMapping(ClientPtr client)
//{
//    // TODO: char *XGLVQueryXIDVendorMapping(XID xid)
//    // Returns the name of the vendor library for this XID
//}

static int ProcGLVQueryXIDScreenMapping(ClientPtr client)
{
    xglvQueryXIDScreenMappingReply rep;
    REQUEST(xglvQueryXIDScreenMappingReq);
    int scrnum;

    REQUEST_SIZE_MATCH(*stuff);

    scrnum = LookupXIDScreenMapping(client, stuff->xid);

    // Write the reply
    GLVND_REPLY_HEADER(rep, 0);
    rep.screen = scrnum;

    WriteToClient(client, sz_xglvQueryXIDScreenMappingReply, (char *)&rep);
    return client->noClientException;
}

static int ProcGLVQueryScreenVendorMapping(ClientPtr client)
{
    xglvQueryScreenVendorMappingReply rep;
    REQUEST(xglvQueryScreenVendorMappingReq);
    const char *vendor;
    size_t n, length;
    char *buf;
    ScreenPtr pScreen;
    XGLVScreenPriv *pScreenPriv;

    REQUEST_SIZE_MATCH(*stuff);

    if ((stuff->screen >= screenInfo.numScreens) ||
        (stuff->screen < 0)) {
        vendor = NULL;
    } else {
        pScreen = screenInfo.screens[stuff->screen];
        pScreenPriv = XGLV_SCREEN_PRIVATE(pScreen);
        vendor = pScreenPriv->vendorLib;
    }

    if (vendor) {
        n = strlen(vendor) + 1;
        length = GLVND_PAD(n) >> 2;
        buf = malloc(length << 2);
        if (!buf) {
            return BadAlloc;
        }
        strncpy(buf, vendor, n);

        // Write the reply
        GLVND_REPLY_HEADER(rep, length);
        rep.n = n;

        WriteToClient(client, sz_xglvQueryScreenVendorMappingReply, (char *)&rep);
        WriteToClient(client, (int)(length << 2), buf);

        free(buf);
    } else {
        GLVND_REPLY_HEADER(rep, 0);
        rep.n = 0;
        WriteToClient(client, sz_xglvQueryScreenVendorMappingReply, (char *)&rep);
    }

    return client->noClientException;
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
    size_t i;
    XGLVDrawableType *drawType;

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

    XGLV_INIT_PRIVATE_SPACE();

    for (i = 0; i < screenInfo.numScreens; i++) {
        xglvScreenInit(screenInfo.screens[i]);
    }

    glvnd_list_init(&xglvDrawableTypes);
    drawType = malloc(sizeof(*drawType));
    drawType->rtype = RT_WINDOW;
    glvnd_list_add(&drawType->entry, &xglvDrawableTypes);
}

