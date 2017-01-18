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

void handleClientConn(void *data) {
    int fd = (int)data;
    EGLStreamKHR stream;
    GLuint texId;

    stream = eglCreateStreamFromFileDescriptorKHR(__eglDpy, fd);

    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texId);
    eglStreamConsumerGLTextureExternalKHR(__eglDpy, stream);

    sampleSurfaceListAppend(__surfaceList, stream, texId);

    /* Rest of the client connection handling */
    [...]
}

void handleClientRepaint(void *data) {
    int id = (int)data;

    forall (surf, __surfaceList) {
        if (surf->id == id) {
            eglStreamConsumerAcquireKHR(__eglDpy, surf->stream);
            break;
        }
    }

    /* Rest of the client repaint handling */
    [...]
}

int main() {
    /* Window system setup */
    __sampleDpy = sample_create_display("sample-display");

    sample_listen_to_client_connections(__sampleDpy, handleClientConn);
    sample_listen_to_client_repaints(__sampleDpy, handleClientRepaint);

    /* Rest of the window system setup */
    [...]

    /* Native EGL setup (EGLDevice, GBM device, ...) */
    __nativeDpy = /* Get native EGL display/device */

    /* Get an EGLDisplay supporting EGLStreams that would allow creation of
       scanout EGLSurfaces */
    __eglDpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_NATIVE, __nativeDpy, NULL);
    eglInitialize(__eglDpy);

    __eglSurf = /* Create a scanout EGLSurface on __eglDpy */;
    __eglCtx  = eglCreateContext(__eglDpy, <EGL config>, EGL_NO_CONTEXT, NULL);

    eglMakeCurrent(__eglDpy, __eglSurf, __eglSurf, __eglCtx);

    /* Rest of the EGL and OpenGL setup */
    [...]

    while (1) {
        sample_dispatch_events(__sampleDpy);

        /* Composite, repaint, ... */
        [...]

        forall (surf, __surfaceList) {
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, surf->texId);
            /* Draw surface quad */
        }

        /* Rest of the repaint handling */
        [...]

        eglSwapBuffers(__eglDpy, eglSrf);
    }

    /* Cleanup */
    [...]
}
