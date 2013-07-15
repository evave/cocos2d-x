/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <dlfcn.h>
#include <android/log.h>
#include <jni/JniHelper.h>
#include <jni.h>
#include <GLES2/gl2.h>
#include <stdlib.h>
#include "AndroidGraphicBuffer.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "AndroidGraphicBuffer" , ## args)

#define  CLASS_NAME   "org/cocos2dx/lib/Cocos2dxHelper"
#define  METHOD_NAME  "getDeviceModel"

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES                                 0x8D65
#endif

#define EGL_NATIVE_BUFFER_ANDROID 0x3140
#define EGL_IMAGE_PRESERVED_KHR   0x30D2

typedef void *EGLContext;
typedef void *EGLDisplay;
typedef uint32_t EGLenum;
typedef int32_t EGLint;
typedef uint32_t EGLBoolean;

#define EGL_TRUE 1
#define EGL_FALSE 0
#define EGL_NONE 0x3038
#define EGL_NO_CONTEXT (EGLContext)0
#define EGL_DEFAULT_DISPLAY  (void*)0

#define ANDROID_LIBUI_PATH "libui.so"
#define ANDROID_GLES_PATH "libGLESv2.so"
#define ANDROID_EGL_PATH "libEGL.so"

// Really I have no idea, but this should be big enough
#define GRAPHIC_BUFFER_SIZE 1024

enum {
    /* buffer is never read in software */
    GRALLOC_USAGE_SW_READ_NEVER   = 0x00000000,
    /* buffer is rarely read in software */
    GRALLOC_USAGE_SW_READ_RARELY  = 0x00000002,
    /* buffer is often read in software */
    GRALLOC_USAGE_SW_READ_OFTEN   = 0x00000003,
    /* mask for the software read values */
    GRALLOC_USAGE_SW_READ_MASK    = 0x0000000F,

    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_NEVER  = 0x00000000,
    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_RARELY = 0x00000020,
    /* buffer is never written in software */
    GRALLOC_USAGE_SW_WRITE_OFTEN  = 0x00000030,
    /* mask for the software write values */
    GRALLOC_USAGE_SW_WRITE_MASK   = 0x000000F0,

    /* buffer will be used as an OpenGL ES texture */
    GRALLOC_USAGE_HW_TEXTURE      = 0x00000100,
    /* buffer will be used as an OpenGL ES render target */
    GRALLOC_USAGE_HW_RENDER       = 0x00000200,
    /* buffer will be used by the 2D hardware blitter */
    GRALLOC_USAGE_HW_2D           = 0x00000400,
    /* buffer will be used with the framebuffer device */
    GRALLOC_USAGE_HW_FB           = 0x00001000,
    /* mask for the software usage bit-mask */
    GRALLOC_USAGE_HW_MASK         = 0x00001F00,
};

enum {
    HAL_PIXEL_FORMAT_RGBA_8888          = 1,
    HAL_PIXEL_FORMAT_RGBX_8888          = 2,
    HAL_PIXEL_FORMAT_RGB_888            = 3,
    HAL_PIXEL_FORMAT_RGB_565            = 4,
    HAL_PIXEL_FORMAT_BGRA_8888          = 5,
    HAL_PIXEL_FORMAT_RGBA_5551          = 6,
    HAL_PIXEL_FORMAT_RGBA_4444          = 7,
};

typedef struct ARect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} ARect;

static bool gTryRealloc = true;

static class GLFunctions
{
public:
  GLFunctions() : mInitialized(false)
  {
  }

  typedef EGLDisplay (* pfnGetDisplay)(void *display_id);
  pfnGetDisplay fGetDisplay;
  typedef EGLint (* pfnEGLGetError)(void);
  pfnEGLGetError fEGLGetError;

  typedef EGLImageKHR (* pfnCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
  pfnCreateImageKHR fCreateImageKHR;
  typedef EGLBoolean (* pfnDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);
  pfnDestroyImageKHR fDestroyImageKHR;

  typedef void (* pfnImageTargetTexture2DOES)(GLenum target, EGLImageKHR image);
  pfnImageTargetTexture2DOES fImageTargetTexture2DOES;

  typedef void (* pfnBindTexture)(GLenum target, GLuint texture);
  pfnBindTexture fBindTexture;

  typedef GLenum (* pfnGLGetError)();
  pfnGLGetError fGLGetError;

  typedef void (*pfnGraphicBufferCtor)(void*, uint32_t w, uint32_t h, uint32_t format, uint32_t usage);
  pfnGraphicBufferCtor fGraphicBufferCtor;

  typedef void (*pfnGraphicBufferDtor)(void*);
  pfnGraphicBufferDtor fGraphicBufferDtor;

  typedef int (*pfnGraphicBufferLock)(void*, uint32_t usage, unsigned char **addr);
  pfnGraphicBufferLock fGraphicBufferLock;

  typedef int (*pfnGraphicBufferLockRect)(void*, uint32_t usage, const ARect&, unsigned char **addr);
  pfnGraphicBufferLockRect fGraphicBufferLockRect;

  typedef int (*pfnGraphicBufferUnlock)(void*);
  pfnGraphicBufferUnlock fGraphicBufferUnlock;

  typedef void* (*pfnGraphicBufferGetNativeBuffer)(void*);
  pfnGraphicBufferGetNativeBuffer fGraphicBufferGetNativeBuffer;

  typedef int (*pfnGraphicBufferReallocate)(void*, uint32_t w, uint32_t h, uint32_t format);
  pfnGraphicBufferReallocate fGraphicBufferReallocate;

  bool EnsureInitialized()
  {
    if (mInitialized) {
      return true;
    }

    void *handle = dlopen(ANDROID_EGL_PATH, RTLD_LAZY);
    if (!handle) {
      LOG("Couldn't load EGL library");
      return false;
    }

    fGetDisplay = (pfnGetDisplay)dlsym(handle, "eglGetDisplay");
    fEGLGetError = (pfnEGLGetError)dlsym(handle, "eglGetError");
    fCreateImageKHR = (pfnCreateImageKHR)dlsym(handle, "eglCreateImageKHR");
    fDestroyImageKHR = (pfnDestroyImageKHR)dlsym(handle, "eglDestroyImageKHR");

    if (!fGetDisplay || !fEGLGetError || !fCreateImageKHR || !fDestroyImageKHR) {
        LOG("Failed to find some EGL functions");
        return false;
    }

    handle = dlopen(ANDROID_GLES_PATH, RTLD_LAZY);
    if (!handle) {
        LOG("Couldn't load GL library");
        return false;
    }

    fImageTargetTexture2DOES = (pfnImageTargetTexture2DOES)dlsym(handle, "glEGLImageTargetTexture2DOES");
    fBindTexture = (pfnBindTexture)dlsym(handle, "glBindTexture");
    fGLGetError = (pfnGLGetError)dlsym(handle, "glGetError");

    if (!fImageTargetTexture2DOES || !fBindTexture || !fGLGetError) {
        LOG("Failed to find some GL functions");
        return false;
    }

    handle = dlopen(ANDROID_LIBUI_PATH, RTLD_LAZY);
    if (!handle) {
        LOG("Couldn't load libui.so");
        return false;
    }

    fGraphicBufferCtor = (pfnGraphicBufferCtor)dlsym(handle, "_ZN7android13GraphicBufferC1Ejjij");
    fGraphicBufferDtor = (pfnGraphicBufferDtor)dlsym(handle, "_ZN7android13GraphicBufferD1Ev");
    fGraphicBufferLock = (pfnGraphicBufferLock)dlsym(handle, "_ZN7android13GraphicBuffer4lockEjPPv");
    fGraphicBufferLockRect = (pfnGraphicBufferLockRect)dlsym(handle, "_ZN7android13GraphicBuffer4lockEjRKNS_4RectEPPv");
    fGraphicBufferUnlock = (pfnGraphicBufferUnlock)dlsym(handle, "_ZN7android13GraphicBuffer6unlockEv");
    fGraphicBufferGetNativeBuffer = (pfnGraphicBufferGetNativeBuffer)dlsym(handle, "_ZNK7android13GraphicBuffer15getNativeBufferEv");
    fGraphicBufferReallocate = (pfnGraphicBufferReallocate)dlsym(handle, "_ZN7android13GraphicBuffer10reallocateEjjij");

    if (!fGraphicBufferCtor || !fGraphicBufferDtor || !fGraphicBufferLock ||
        !fGraphicBufferUnlock || !fGraphicBufferGetNativeBuffer) {
        LOG("Failed to lookup some GraphicBuffer functions");
        return false;
    }

    mInitialized = true;
    return true;
  }

private:
  bool mInitialized;

} sGLFunctions;

namespace cocos2d {

static void clearGLError()
{
    while (glGetError() != GL_NO_ERROR);
}

static bool ensureNoGLError(const char* name)
{
    bool result = true;
    GLuint error;

    while ((error = glGetError()) != GL_NO_ERROR) {
        LOG("GL error [%s]: %40x\n", name, error);
        result = false;
    }

    return result;
}

AndroidGraphicBuffer::AndroidGraphicBuffer(uint32_t width, uint32_t height, uint32_t usage,
                                           CCTexture2DPixelFormat format) :
    mWidth(width)
  , mHeight(height)
  , mUsage(usage)
  , mFormat(format)
  , mHandle(0)
  , mEGLImage(0)
{
}

AndroidGraphicBuffer::~AndroidGraphicBuffer()
{
    DestroyBuffer();
}

void AndroidGraphicBuffer::DestroyBuffer()
{
    /**
     * XXX: eglDestroyImageKHR crashes sometimes due to refcount badness (I think)
     *
     * If you look at egl.cpp (https://github.com/android/platform_frameworks_base/blob/master/opengl/libagl/egl.cpp#L2002)
     * you can see that eglCreateImageKHR just refs the native buffer, and eglDestroyImageKHR
     * just unrefs it. Somehow the ref count gets messed up and things are already destroyed
     * by the time eglDestroyImageKHR gets called. For now, at least, just not calling
     * eglDestroyImageKHR should be fine since we do free the GraphicBuffer below.
     *
     * Bug 712716
     */
#if 0
    if (mEGLImage) {
        if (sGLFunctions.EnsureInitialized()) {
            sGLFunctions.fDestroyImageKHR(sGLFunctions.fGetDisplay(EGL_DEFAULT_DISPLAY), mEGLImage);
            mEGLImage = NULL;
        }
    }
#endif
    mEGLImage = NULL;

    if (mHandle) {
        if (sGLFunctions.EnsureInitialized()) {
            sGLFunctions.fGraphicBufferDtor(mHandle);
        }
        free(mHandle);
        mHandle = NULL;
    }
}

bool AndroidGraphicBuffer::EnsureBufferCreated()
{
    if (!mHandle) {
        mHandle = malloc(GRAPHIC_BUFFER_SIZE);
        sGLFunctions.fGraphicBufferCtor(mHandle, mWidth, mHeight, GetAndroidFormat(mFormat), GetAndroidUsage(mUsage));
    }

    return true;
}

bool AndroidGraphicBuffer::EnsureInitialized()
{
    if (!sGLFunctions.EnsureInitialized()) {
        return false;
    }

    EnsureBufferCreated();
    return true;
}

int AndroidGraphicBuffer::Lock(uint32_t aUsage, unsigned char **bits)
{
    if (!EnsureInitialized())
        return true;

    return sGLFunctions.fGraphicBufferLock(mHandle, GetAndroidUsage(aUsage), bits);
}

int AndroidGraphicBuffer::Lock(uint32_t aUsage, const CCRect& aRect, unsigned char **bits)
{
    if (!EnsureInitialized())
        return false;

    ARect rect;
    rect.left = aRect.origin.x;
    rect.top = aRect.origin.y;
    rect.right = aRect.origin.x + aRect.size.width;
    rect.bottom = aRect.origin.y + aRect.size.height;

    return sGLFunctions.fGraphicBufferLockRect(mHandle, GetAndroidUsage(aUsage), rect, bits);
}

int AndroidGraphicBuffer::Unlock()
{
    if (!EnsureInitialized())
        return false;

    return sGLFunctions.fGraphicBufferUnlock(mHandle);
}

bool AndroidGraphicBuffer::Reallocate(uint32_t aWidth, uint32_t aHeight, CCTexture2DPixelFormat aFormat)
{
    if (!EnsureInitialized())
        return false;

    mWidth = aWidth;
    mHeight = aHeight;
    mFormat = aFormat;

    // Sometimes GraphicBuffer::reallocate just doesn't work. In those cases we'll just allocate a brand
    // new buffer. If reallocate fails once, never try it again.
    if (!gTryRealloc || sGLFunctions.fGraphicBufferReallocate(mHandle, aWidth, aHeight, GetAndroidFormat(aFormat)) != 0) {
        DestroyBuffer();
        EnsureBufferCreated();

        gTryRealloc = false;
    }

    return true;
}

uint32_t AndroidGraphicBuffer::GetAndroidUsage(uint32_t aUsage)
{
    uint32_t flags = 0;

    if (aUsage & UsageSoftwareRead) {
        flags |= GRALLOC_USAGE_SW_READ_OFTEN;
    }

    if (aUsage & UsageSoftwareWrite) {
        flags |= GRALLOC_USAGE_SW_WRITE_OFTEN;
    }

    if (aUsage & UsageTexture) {
        flags |= GRALLOC_USAGE_HW_TEXTURE;
    }

    if (aUsage & UsageTarget) {
        flags |= GRALLOC_USAGE_HW_RENDER;
    }

    if (aUsage & Usage2D) {
        flags |= GRALLOC_USAGE_HW_2D;
    }

    if (aUsage & UsageFramebuffer) {
        flags |= GRALLOC_USAGE_HW_FB;
    }

    return flags;
}

uint32_t AndroidGraphicBuffer::GetAndroidFormat(CCTexture2DPixelFormat aFormat)
{
    switch (aFormat) {
    case kCCTexture2DPixelFormat_RGBA8888:
        return HAL_PIXEL_FORMAT_RGBA_8888;
    case kCCTexture2DPixelFormat_RGB888:
        return HAL_PIXEL_FORMAT_RGB_888;
    case kCCTexture2DPixelFormat_RGB565:
        return HAL_PIXEL_FORMAT_RGB_565;
    case kCCTexture2DPixelFormat_RGB5A1:
        return HAL_PIXEL_FORMAT_RGBA_5551;
    case kCCTexture2DPixelFormat_RGBA4444:
        return HAL_PIXEL_FORMAT_RGBA_4444;
    default:
        return 0;
    }
}

bool AndroidGraphicBuffer::EnsureEGLImage()
{
    if (mEGLImage)
        return true;

    if (!EnsureInitialized())
        return false;

    EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
    void* nativeBuffer = sGLFunctions.fGraphicBufferGetNativeBuffer(mHandle);

    mEGLImage = sGLFunctions.fCreateImageKHR(sGLFunctions.fGetDisplay(EGL_DEFAULT_DISPLAY), EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)nativeBuffer, eglImgAttrs);
    return mEGLImage != NULL;
}

bool AndroidGraphicBuffer::Bind()
{
    if (!EnsureInitialized())
        return false;

    if (!EnsureEGLImage()) {
        LOG("No valid EGLImage!");
        return false;
    }

    clearGLError();
    sGLFunctions.fImageTargetTexture2DOES(GL_TEXTURE_2D, mEGLImage);
//    sGLFunctions.fImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, mEGLImage);
    return ensureNoGLError("glEGLImageTargetTexture2DOES");
}

// NOTE: Bug with glReadPixels() appears only on some models, so we enabled it
// and disabled other devices for cocos2d-x.
// See also https://en.wikipedia.org/wiki/Samsung_Galaxy_S_III#Model_variants
// Look for Samsung S3 models with Qualcomm Adreno 225 GPU.
static const char* const sAllowedBoards[] = {
    "GT-I9300",     // Samsung Galaxy S3 GT-I9300 720 x 1280
    "SPH-L710",     // Samsung Galaxy S3 Sprint SPH-L710 720 x 1280
    "SGH-T999",     // Samsung Galaxy S III T-Mobile
    "SGH-N064",
    "SCH-J021",
    "SCH-R530",
    "SCH-I535",
    "SCH-I535",
//    "venus2",     // Motorola Droid Pro
//    "tuna",       // Galaxy Nexus
//    "omap4sdp",   // Amazon Kindle Fire
//    "droid2",     // Motorola Droid 2
//    "targa",      // Motorola Droid Bionic
//    "spyder",     // Motorola Razr
//    "shadow",     // Motorola Droid X
//    "SGH-I897",   // Samsung Galaxy S
//    "GT-I9100",   // Samsung Galaxy SII
//    "sgh-i997",   // Samsung Infuse 4G
//    "herring",    // Samsung Nexus S
//    "sgh-t839",   // Samsung Sidekick 4G
    NULL
};

bool AndroidGraphicBuffer::IsBlacklisted(std::string &deviceModel)
{
    JniMethodInfo methodInfo;
    jstring jstr;
    if (JniHelper::getStaticMethodInfo(methodInfo, CLASS_NAME, METHOD_NAME, "()Ljava/lang/String;"))
    {
        jstr = (jstring)methodInfo.env->CallStaticObjectMethod(methodInfo.classID, methodInfo.methodID);
    }
    methodInfo.env->DeleteLocalRef(methodInfo.classID);
    deviceModel = methodInfo.env->GetStringUTFChars(jstr, NULL);

    // FIXME: (Bug 722605) use something better than a linear search
    for (int i = 0; sAllowedBoards[i]; i++)
        if (deviceModel.find(sAllowedBoards[i]) != std::string::npos)
            return false;

    return true;
}

} /* cocos2d */

