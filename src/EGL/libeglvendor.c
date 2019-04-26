#include "libeglvendor.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <fnmatch.h>
#include <dirent.h>

#include "glvnd_pthread.h"
#include "libeglcurrent.h"
#include "libeglmapping.h"
#include "utils_misc.h"
#include "glvnd_list.h"
#include "cJSON.h"
#include "egldispatchstubs.h"

#define FILE_FORMAT_VERSION_MAJOR 1
#define FILE_FORMAT_VERSION_MINOR 0

static void LoadVendors(void);
static void TeardownVendor(__EGLvendorInfo *vendor);
static __EGLvendorInfo *LoadVendor(const char *filename);

static void LoadVendorsFromConfigDir(const char *dirName);
static __EGLvendorInfo *LoadVendorFromConfigFile(const char *filename);
static cJSON *ReadJSONFile(const char *filename);

static glvnd_once_t loadVendorsOnceControl = GLVND_ONCE_INIT;
static struct glvnd_list __eglVendorList;

void LoadVendors(void)
{
    const char *env = NULL;
    char **tokens;
    int i;

    // First, check to see if a list of vendors was specified.
    if (getuid() == geteuid() && getgid() == getegid()) {
        env = getenv("__EGL_VENDOR_LIBRARY_FILENAMES");
    }
    if (env != NULL) {
        tokens = SplitString(env, NULL, ":");
        if (tokens != NULL) {
            for (i=0; tokens[i] != NULL; i++) {
                LoadVendorFromConfigFile(tokens[i]);
            }
            free(tokens);
        }
        return;
    }

    // We didn't get a list of vendors, so look through the vendor config
    // directories.
    if (getuid() == geteuid() && getgid() == getegid()) {
        env = getenv("__EGL_VENDOR_LIBRARY_DIRS");
    }
    if (env == NULL) {
        env = DEFAULT_EGL_VENDOR_CONFIG_DIRS;
    }

    tokens = SplitString(env, NULL, ":");
    if (tokens != NULL) {
        for (i=0; tokens[i] != NULL; i++) {
            LoadVendorsFromConfigDir(tokens[i]);
        }
        free(tokens);
    }
}

static int ScandirFilter(const struct dirent *ent)
{
#if defined(HAVE_DIRENT_DTYPE)
    // Ignore the entry if we know that it's not a regular file or symlink.
    if (ent->d_type != DT_REG && ent->d_type != DT_LNK && ent->d_type != DT_UNKNOWN) {
        return 0;
    }
#endif

    // Otherwise, select any JSON files.
    if (fnmatch("*.json", ent->d_name, 0) == 0) {
        return 1;
    } else {
        return 0;
    }
}

static int CompareFilenames(const struct dirent **ent1, const struct dirent **ent2)
{
    return strcmp((*ent1)->d_name, (*ent2)->d_name);
}

void LoadVendorsFromConfigDir(const char *dirName)
{
    struct dirent **entries = NULL;
    size_t dirnameLen;
    const char *pathSep;
    int count;
    int i;

    count = scandir(dirName, &entries, ScandirFilter, CompareFilenames);
    if (count <= 0) {
        return;
    }

    // Check if dirName ends with a "/" character. If it doesn't, then we need
    // to add one when we construct the full file paths below.
    dirnameLen = strlen(dirName);
    if (dirnameLen > 0 && dirName[dirnameLen - 1] != '/') {
        pathSep = "/";
    } else {
        pathSep = "";
    }

    for (i=0; i<count; i++) {
        char *path = NULL;
        if (glvnd_asprintf(&path, "%s%s%s", dirName, pathSep, entries[i]->d_name) > 0) {
            LoadVendorFromConfigFile(path);
            free(path);
        } else {
            fprintf(stderr, "ERROR: Could not allocate vendor library path name\n");
        }
        free(entries[i]);
    }

    free(entries);
}

void __eglInitVendors(void)
{
    glvnd_list_init(&__eglVendorList);
}

struct glvnd_list *__eglLoadVendors(void)
{
    __glvndPthreadFuncs.once(&loadVendorsOnceControl, LoadVendors);
    return &__eglVendorList;
}

void __eglTeardownVendors(void)
{
    __EGLvendorInfo *vendor;
    __EGLvendorInfo *vendorTemp;

    glvnd_list_for_each_entry_safe(vendor, vendorTemp, &__eglVendorList, entry) {
        glvnd_list_del(&vendor->entry);
        __glDispatchForceUnpatch(vendor->vendorID);
        TeardownVendor(vendor);
    }
}

const __EGLapiExports __eglExportsTable = {
    __eglThreadInitialize, // threadInit
    __eglQueryAPI, // getCurrentApi
    __eglGetCurrentVendor, // getCurrentVendor
    __eglGetCurrentContext, // getCurrentContext
    __eglGetCurrentDisplay, // getCurrentDisplay
    __eglGetCurrentSurface, // getCurrentSurface
    __eglFetchDispatchEntry, // fetchDispatchEntry
    __eglSetError, // setEGLError
    __eglSetLastVendor, // setLastVendor
    __eglGetVendorFromDisplay, // getVendorFromDisplay
    __eglGetVendorFromDevice, // getVendorFromDevice
};

void TeardownVendor(__EGLvendorInfo *vendor)
{
    if (vendor->glDispatch) {
        __glDispatchDestroyTable(vendor->glDispatch);
    }

    /* Clean up the dynamic dispatch table */
    if (vendor->dynDispatch != NULL) {
        __glvndWinsysVendorDispatchDestroy(vendor->dynDispatch);
        vendor->dynDispatch = NULL;
    }

    if (vendor->dlhandle != NULL) {
        dlclose(vendor->dlhandle);
    }

    free(vendor);
}

static GLboolean LookupVendorEntrypoints(__EGLvendorInfo *vendor)
{
    memset(&vendor->staticDispatch, 0, sizeof(vendor->staticDispatch));

    // TODO: A lot of these should be implemented (and probably generated) as
    // normal EGL dispatch functions, instead of having to special-case them.

#define LOADENTRYPOINT(ptr, name) do { \
    vendor->staticDispatch.ptr = vendor->eglvc.getProcAddress(name); \
    if (vendor->staticDispatch.ptr == NULL) { return GL_FALSE; } \
    } while(0)

    LOADENTRYPOINT(initialize,                    "eglInitialize"                    );
    LOADENTRYPOINT(chooseConfig,                  "eglChooseConfig"                  );
    LOADENTRYPOINT(copyBuffers,                   "eglCopyBuffers"                   );
    LOADENTRYPOINT(createContext,                 "eglCreateContext"                 );
    LOADENTRYPOINT(createPbufferSurface,          "eglCreatePbufferSurface"          );
    LOADENTRYPOINT(createPixmapSurface,           "eglCreatePixmapSurface"           );
    LOADENTRYPOINT(createWindowSurface,           "eglCreateWindowSurface"           );
    LOADENTRYPOINT(destroyContext,                "eglDestroyContext"                );
    LOADENTRYPOINT(destroySurface,                "eglDestroySurface"                );
    LOADENTRYPOINT(getConfigAttrib,               "eglGetConfigAttrib"               );
    LOADENTRYPOINT(getConfigs,                    "eglGetConfigs"                    );
    LOADENTRYPOINT(makeCurrent,                   "eglMakeCurrent"                   );
    LOADENTRYPOINT(queryContext,                  "eglQueryContext"                  );
    LOADENTRYPOINT(queryString,                   "eglQueryString"                   );
    LOADENTRYPOINT(querySurface,                  "eglQuerySurface"                  );
    LOADENTRYPOINT(swapBuffers,                   "eglSwapBuffers"                   );
    LOADENTRYPOINT(terminate,                     "eglTerminate"                     );
    LOADENTRYPOINT(waitGL,                        "eglWaitGL"                        );
    LOADENTRYPOINT(waitNative,                    "eglWaitNative"                    );
    LOADENTRYPOINT(bindTexImage,                  "eglBindTexImage"                  );
    LOADENTRYPOINT(releaseTexImage,               "eglReleaseTexImage"               );
    LOADENTRYPOINT(surfaceAttrib,                 "eglSurfaceAttrib"                 );
    LOADENTRYPOINT(swapInterval,                  "eglSwapInterval"                  );
    LOADENTRYPOINT(createPbufferFromClientBuffer, "eglCreatePbufferFromClientBuffer" );
    LOADENTRYPOINT(releaseThread,                 "eglReleaseThread"                 );
    LOADENTRYPOINT(waitClient,                    "eglWaitClient"                    );
    LOADENTRYPOINT(getError,                      "eglGetError"                      );
#undef LOADENTRYPOINT

    // The remaining functions here are optional.
#define LOADENTRYPOINT(ptr, name) \
    vendor->staticDispatch.ptr = vendor->eglvc.getProcAddress(name);

    LOADENTRYPOINT(bindAPI,                       "eglBindAPI"                       );
    LOADENTRYPOINT(createSync,                    "eglCreateSync"                    );
    LOADENTRYPOINT(destroySync,                   "eglDestroySync"                   );
    LOADENTRYPOINT(clientWaitSync,                "eglClientWaitSync"                );
    LOADENTRYPOINT(getSyncAttrib,                 "eglGetSyncAttrib"                 );
    LOADENTRYPOINT(createImage,                   "eglCreateImage"                   );
    LOADENTRYPOINT(destroyImage,                  "eglDestroyImage"                  );
    LOADENTRYPOINT(createPlatformWindowSurface,   "eglCreatePlatformWindowSurface"   );
    LOADENTRYPOINT(createPlatformPixmapSurface,   "eglCreatePlatformPixmapSurface"   );
    LOADENTRYPOINT(waitSync,                      "eglWaitSync"                      );
    LOADENTRYPOINT(queryDevicesEXT,               "eglQueryDevicesEXT"               );

    LOADENTRYPOINT(debugMessageControlKHR,        "eglDebugMessageControlKHR"        );
    LOADENTRYPOINT(queryDebugKHR,                 "eglQueryDebugKHR"                 );
    LOADENTRYPOINT(labelObjectKHR,                "eglLabelObjectKHR"                );
#undef LOADENTRYPOINT

    return GL_TRUE;
}

static void *VendorGetProcAddressCallback(const char *procName, void *param)
{
    __EGLvendorInfo *vendor = (__EGLvendorInfo *) param;
    return vendor->eglvc.getProcAddress(procName);
}

static EGLBoolean CheckFormatVersion(const char *versionStr)
{
    int major, minor, rev;
    int len;

    major = minor = rev = -1;
    len = sscanf(versionStr, "%d.%d.%d", &major, &minor, &rev);
    if (len < 1) {
        return EGL_FALSE;
    }
    if (len < 2) {
        minor = 0;
    }
    if (len < 3) {
        rev = 0;
    }
    if (major != FILE_FORMAT_VERSION_MAJOR) {
        return EGL_FALSE;
    }

    // The minor version number will be incremented if we ever add an optional
    // value to the JSON format that libEGL has to pay attention to. That is,
    // an older vendor library will still work, but a vendor library with a
    // newer format than this library understands should fail.
    if (minor > FILE_FORMAT_VERSION_MINOR) {
        return EGL_FALSE;
    }
    return EGL_TRUE;
}

static __EGLvendorInfo *LoadVendorFromConfigFile(const char *filename)
{
    __EGLvendorInfo *vendor = NULL;
    cJSON *root;
    cJSON *node;
    cJSON *icdNode;
    const char *libraryPath;

    root = ReadJSONFile(filename);
    if (root == NULL) {
        goto done;
    }

    node = cJSON_GetObjectItem(root, "file_format_version");
    if (node == NULL || node->type != cJSON_String) {
        goto done;
    }
    if (!CheckFormatVersion(node->valuestring)) {
        goto done;
    }

    icdNode = cJSON_GetObjectItem(root, "ICD");
    if (icdNode == NULL || icdNode->type != cJSON_Object) {
        goto done;
    }

    node = cJSON_GetObjectItem(icdNode, "library_path");
    if (node == NULL || node->type != cJSON_String) {
        goto done;
    }
    libraryPath = node->valuestring;
    vendor = LoadVendor(libraryPath);

done:
    if (root != NULL) {
        cJSON_Delete(root);
    }
    if (vendor != NULL) {
        glvnd_list_append(&vendor->entry, &__eglVendorList);
    }
    return vendor;
}

static cJSON *ReadJSONFile(const char *filename)
{
    FILE *in = NULL;
    char *buf = NULL;
    cJSON *root = NULL;
    struct stat st;

    in = fopen(filename, "r");
    if (in == NULL) {
        goto done;
    }

    if (fstat(fileno(in), &st) != 0) {
        goto done;
    }

    buf = (char *) malloc(st.st_size + 1);
    if (buf == NULL) {
        goto done;
    }

    if (fread(buf, st.st_size, 1, in) != 1) {
        goto done;
    }
    buf[st.st_size] = '\0';

    root = cJSON_Parse(buf);

done:
    if (in != NULL) {
        fclose(in);
    }
    if (buf != NULL) {
        free(buf);
    }
    return root;
}

static void CheckVendorExtensionString(__EGLvendorInfo *vendor, const char *str)
{
    static const char NAME_DEVICE_BASE[] = "EGL_EXT_device_base";
    static const char NAME_DEVICE_ENUM[] = "EGL_EXT_device_enumeration";
    static const char NAME_PLATFORM_DEVICE[] = "EGL_EXT_platform_device";
    static const char NAME_MESA_PLATFORM_GBM[] = "EGL_MESA_platform_gbm";
    static const char NAME_KHR_PLATFORM_GBM[] = "EGL_KHR_platform_gbm";
    static const char NAME_EXT_PLATFORM_WAYLAND[] = "EGL_EXT_platform_wayland";
    static const char NAME_KHR_PLATFORM_WAYLAND[] = "EGL_KHR_platform_wayland";
    static const char NAME_EXT_PLATFORM_X11[] = "EGL_EXT_platform_x11";
    static const char NAME_KHR_PLATFORM_X11[] = "EGL_KHR_platform_x11";

    if (str == NULL || str[0] == '\x00') {
        return;
    }

    if (!vendor->supportsDevice) {
        if (IsTokenInString(str, NAME_DEVICE_BASE, sizeof(NAME_DEVICE_BASE) - 1, " ")
                || IsTokenInString(str, NAME_DEVICE_ENUM, sizeof(NAME_DEVICE_ENUM) - 1, " ")) {
            vendor->supportsDevice = EGL_TRUE;
        }
    }

    if (!vendor->supportsPlatformDevice) {
        if (IsTokenInString(str, NAME_PLATFORM_DEVICE, sizeof(NAME_PLATFORM_DEVICE) - 1, " ")) {
            vendor->supportsPlatformDevice = EGL_TRUE;
        }
    }

    if (!vendor->supportsPlatformGbm) {
        if (IsTokenInString(str, NAME_MESA_PLATFORM_GBM, sizeof(NAME_MESA_PLATFORM_GBM) - 1, " ")
                || IsTokenInString(str, NAME_KHR_PLATFORM_GBM, sizeof(NAME_KHR_PLATFORM_GBM) - 1, " ")) {
            vendor->supportsPlatformGbm = EGL_TRUE;
        }
    }

    if (!vendor->supportsPlatformWayland) {
        if (IsTokenInString(str, NAME_EXT_PLATFORM_WAYLAND, sizeof(NAME_EXT_PLATFORM_WAYLAND) - 1, " ")
                || IsTokenInString(str, NAME_KHR_PLATFORM_WAYLAND, sizeof(NAME_KHR_PLATFORM_WAYLAND) - 1, " ")) {
            vendor->supportsPlatformWayland = EGL_TRUE;
        }
    }

    if (!vendor->supportsPlatformX11) {
        if (IsTokenInString(str, NAME_EXT_PLATFORM_X11, sizeof(NAME_EXT_PLATFORM_X11) - 1, " ")
                || IsTokenInString(str, NAME_KHR_PLATFORM_X11, sizeof(NAME_KHR_PLATFORM_X11) - 1, " ")) {
            vendor->supportsPlatformX11 = EGL_TRUE;
        }
    }
}

static void CheckVendorExtensions(__EGLvendorInfo *vendor)
{
    CheckVendorExtensionString(vendor,
            vendor->staticDispatch.queryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));

    if (vendor->eglvc.getVendorString != NULL) {
        CheckVendorExtensionString(vendor,
                vendor->eglvc.getVendorString(__EGL_VENDOR_STRING_PLATFORM_EXTENSIONS));
    }

    if (vendor->staticDispatch.queryDevicesEXT == NULL) {
        vendor->supportsDevice = EGL_FALSE;
    }

    if (!vendor->supportsDevice) {
        vendor->supportsPlatformDevice = EGL_FALSE;
    }
}

static __EGLvendorInfo *LoadVendor(const char *filename)
{
    __PFNEGLMAINPROC eglMainProc;
    __EGLvendorInfo *vendor = NULL;
    __EGLvendorInfo *otherVendor;
    int i;

    // Allocate the vendor structure, plus enough room for a copy of its name.
    vendor = (__EGLvendorInfo *) calloc(1, sizeof(__EGLvendorInfo));
    if (vendor == NULL) {
        return NULL;
    }

    vendor->dlhandle = dlopen(filename, RTLD_LAZY);
    if (vendor->dlhandle == NULL) {
        goto fail;
    }

    // Check if this vendor was already loaded under a different name.
    glvnd_list_for_each_entry(otherVendor, &__eglVendorList, entry) {
        if (otherVendor->dlhandle == vendor->dlhandle) {
            goto fail;
        }
    }

    eglMainProc = dlsym(vendor->dlhandle, __EGL_MAIN_PROTO_NAME);
    if (!eglMainProc) {
        goto fail;
    }

    if (!(*eglMainProc)(EGL_VENDOR_ABI_VERSION,
                              &__eglExportsTable,
                              vendor, &vendor->eglvc)) {
        goto fail;
    }

    // Make sure all the required functions are there.
    if (vendor->eglvc.getPlatformDisplay == NULL
            || vendor->eglvc.getSupportsAPI == NULL
            || vendor->eglvc.getProcAddress == NULL
            || vendor->eglvc.getDispatchAddress == NULL
            || vendor->eglvc.setDispatchIndex == NULL) {
        goto fail;
    }

    if (vendor->eglvc.isPatchSupported != NULL
            && vendor->eglvc.initiatePatch != NULL) {
        vendor->patchCallbacks.isPatchSupported = vendor->eglvc.isPatchSupported;
        vendor->patchCallbacks.initiatePatch = vendor->eglvc.initiatePatch;
        vendor->patchCallbacks.releasePatch = vendor->eglvc.releasePatch;
        vendor->patchCallbacks.threadAttach = vendor->eglvc.patchThreadAttach;
        vendor->patchSupported = EGL_TRUE;
    }

    if (!LookupVendorEntrypoints(vendor)) {
        goto fail;
    }

    vendor->supportsGL = vendor->eglvc.getSupportsAPI(EGL_OPENGL_API);
    vendor->supportsGLES = vendor->eglvc.getSupportsAPI(EGL_OPENGL_ES_API);
    if (!(vendor->supportsGL || vendor->supportsGLES)) {
        goto fail;
    }

    vendor->vendorID = __glDispatchNewVendorID();
    assert(vendor->vendorID >= 0);

    // TODO: Allow per-context dispatch tables?
    vendor->glDispatch = __glDispatchCreateTable(VendorGetProcAddressCallback, vendor);
    if (!vendor->glDispatch) {
        goto fail;
    }

    CheckVendorExtensions(vendor);

    // Create and initialize the EGL dispatch table.
    // This is called before trying to look up any vendor-supplied EGL dispatch
    // functions, so we only need to add the EGL dispatch functions that are
    // defined in libEGL itself.
    vendor->dynDispatch = __glvndWinsysVendorDispatchCreate();
    if (!vendor->dynDispatch) {
        goto fail;
    }
    for (i=0; i<__EGL_DISPATCH_FUNC_COUNT; i++) {
        vendor->eglvc.setDispatchIndex(
                __EGL_DISPATCH_FUNC_NAMES[i],
                __EGL_DISPATCH_FUNC_INDICES[i]);
    }

    return vendor;

fail:
    if (vendor != NULL) {
        TeardownVendor(vendor);
    }
    return NULL;
}

