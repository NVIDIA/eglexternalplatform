/*
 * Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#define SAMPLE_EXTERNAL_VERSION_MAJOR 1
#define SAMPLE_EXTERNAL_VERSION_MINOR 0
#define SAMPLE_EXTERNAL_VERSION_MICRO 123

#define EGL_EXTERNAL_PLATFORM_VERSION_MAJOR SAMPLE_EXTERNAL_VERSION_MAJOR
#define EGL_EXTERNAL_PLATFORM_VERSION_MINOR SAMPLE_EXTERNAL_VERSION_MINOR

#include <eglexternalplatform.h>


typedef const char* (*PSAMPLEEGLFNQUERYSTRINGCOREPROC) (EGLDisplay dpy, EGLint name);
typedef EGLBoolean  (*PSAMPLEEGLFNINITIALIZECOREPROC)  (EGLDisplay dpy, EGLint *major, EGLint *minor);
typedef EGLBoolean  (*PSAMPLEEGLFNTERMINATECOREPROC)   (EGLDisplay dpy);

typedef struct SampleDisplay {
    sample_display     *nativeSampleDpy;

    EGLNativeDisplay    nativeEglDpy; /* EGLDevice, GBM device, ... */
    EGLDisplay          eglDpy;

    SamplePlatformData *sampleData;
} SampleDisplay;

typedef struct SampleSurface {
    sample_window *nativeSampleWin;

    SampleDisplay *sampleDpy;

    EGLStreamKHR   stream;
    EGLSurface     eglSurf;
} SampleSurface;

typedef struct SamplePlatformData {
    /* Application-facing callbacks fetched from the EGL driver */
    struct {
        PSAMPLEEGLFNQUERYSTRINGCOREPROC queryString;

        PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay;
        PSAMPLEEGLFNINITIALIZECOREPROC  initialize;
        PSAMPLEEGLFNTERMINATECOREPROC   terminate;

        /* Other EGL functions */
        [...]
    } egl;

    /* Non-application-facing callbacks provided by the EGL driver */
    struct {
        PEGLEXTFNSETERROR setError;
    } callbacks;
} SamplePlatformData;


EGLDisplay sampleGetPlatformDisplayExport(void *data,
                                          EGLenum platform,
                                          Void *nativeDpy,
                                          Const EGLAttrib *attribs)
{
    SamplePlatformData *sampleData = (SamplePlatformData *)data;
    SampleDisplay      *sampleDpy  = malloc(sizeof(SampleDisplay));

    /* Native EGL setup (EGLDevice, GBM device, ...) */
    sampleDpy->nativeSampleDpy = nativeDpy;
    sampleDpy->nativeEglDpy = /* Get native EGL display/device */

    /* Get an EGLDisplay supporting EGLStreams */
    sampleDpy->eglDpy = sampleData->egl.eglGetPlatformDisplayEXT(EGL_PLATFORM_NATIVE, sampleDpy->nativeEglDpy, NULL);

    sampleDpy->sampleData = sampleData;

    /* Rest of the display creation/allocation */
    [...]

    return (EGLDisplay)sampleDpy;
}

EGLSurface sampleCreatePlatformWindowSurfaceHook(EGLDisplay dpy,
                                                 EGLConfig config,
                                                 void *nativeWin,
                                                 const EGLAttrib *attribs)
{
    SampleDisplay      *sampleDpy  = (SampleDisplay *)dpy;
    SamplePlatformData *sampleData = sampleDpy->sampleData;
    SampleSurface      *sampleSurf = malloc(sizeof(SampleSurface));
    int fd;

    /* Create the underlying EGLStream */
    sampleSurf->stream = sampleData->egl.eglCreateStreamKHR(sampleDpy->eglDpy, NULL);

    /* Let the server create its EGLStream consumer endpoint */
    fd = sampleData->egl.eglGetStreamFileDescriptorKHR(sampleDpy->eglDpy, sampleSurf->stream);
    sample_client_connect(sampleDpy->nativeSampleDpy, fd);

    /* Create a surface producer */
    sampleSurf->nativeSampleWin = nativeWin;
    sampleSurf->eglSurf = sampleData->egl.eglCreateStreamProducerSurfaceKHR(sampleDpy->eglDpy, <EGL config>, sampleSurf->stream, NULL);

    /* Rest of the surface creation/allocation */
    [...]

    return (EGLSurface)sampleSurf;
}

EGLSurface sampleCreatePlatformPixmapSurfaceHook(EGLDisplay dpy,
                                                 EGLConfig config,
                                                 void *nativePixmap,
                                                 const EGLAttrib *attribs)
{
    /* Create pixmap surfae if supported */
    [...]
}

EGLSurface sampleCreatePbufferSurfaceHook(EGLDisplay dpy,
                                          EGLConfig config,
                                          const EGLint *attribs)
{
    /* Create Pbuffer surfae if supported */
    [...]
}

EGLBoolean sampleSwapBuffersHook(EGLDisplay dpy,
                                 EGLSurface surface)
{
    SampleDisplay      *sampleDpy  = (SampleDisplay *)dpy;
    SamplePlatformData *sampleData = sampleDpy->sampleData;
    SampleSurface      *sampleSurf = (SampleSurface *)surface;

    if (sampleSurf->sampleDpy != sampleDpy) {
        sampleData->callbacks.setError(EGL_BAD_SURFACE, EGL_DEBUG_MSG_ERROR_KHR, "Invalid surface");
        return EGL_FALSE;
    }

    sampleData->egl.eglSwapBuffers(sampleDpy->eglDpy, sampleSurf->eglSurf);

    /* Let the server know a new frame was generated */
    sample_client_repaint(sampleDpy->nativeSampleDpy, sampleSurf->nativeSampleWin->id);

    /* Rest of the swap buffers handling */
    [...]

    return EGL_TRUE;
}

static struct {
    const char name[];
    void *func;
} __sampleHooks[] = {
    { "eglCreatePlatformWindowSurface", sampleCreatePlatformWindowSurfaceHook }
    { "eglCreatePlatformPixmapSurface", sampleCreatePlatformPixmapSurfaceHook }
    { "eglCreatePbufferSurface",        sampleCreatePbufferSurfaceHook }
    { "eglSwapBuffers",                 sampleSwapBuffersHook }
};

void* sampleGetHookAddressExport(void *data, const char *name)
{
    forall (hook, __sampleHooks) {
        if (strcmp(hook->name, name) == 0) {
            return hook->func;
        }
    }
    return NULL;
}

EGLBoolean loadEGLExternalPlatform(int major, int minor,
                                   const EGLExtDriver *driver,
                                   EGLExtPlatform *platform)
{
    SamplePlatformData *sampleData = NULL;

    /* Version checking */
    if (!platform ||
        !EGL_EXTERNAL_PLATFORM_VERSION_CHECK(major, minor)) {
        return EGL_FALSE;
    }

    platform->version.major = SAMPLE_EXTERNAL_VERSION_MAJOR;
    platform->version.minor = SAMPLE_EXTERNAL_VERSION_MINOR;
    platform->version.micro = SAMPLE_EXTERNAL_VERSION_MICRO;

    platform->platform = EGL_PLATFORM_SAMPLE;

    /* Allocate and initialize platform private data */
    sampleData = malloc(sizeof(SamplePlatformData));

    sampleData->egl.getPlatformDisplay = driver->getProcAddress("eglGetPlatformDisplayEXT")
    sampleData->egl.initialize         = driver->getProcAddress("eglInitialize")
    sampleData->egl.terminate          = driver->getProcAddress("eglTerminate")

    sampleData->callbacks.setError = driver->setError;

    /* Rest of imports initialization */
    [...]

    /* Fill <platform> with Sample platform data and exports */
    platform->data = (void *)sampleData;
    platform->exports.getHookAddress     = sampleGetHookAddressExport;
    platform->exports.getPlatformDisplay = sampleGetPlatformDisplayExport;

    /* Rest of exports initialization */
    [...]

    return EGL_TRUE;
}
