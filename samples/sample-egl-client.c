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

int main() {
    /* Window system setup */
    __sampleDpy  = sample_display_connect("sample-display");
    __sampleSurf = sample_create_surface(__sampleDpy);

    /* Rest of the window system setup */
    [...]

    /* EGL setup */
    __eglDpy  = eglGetPlatformDisplayEXT(EGL_PLATFORM_SAMPLE, __sampleDpy, NULL);
    __eglSurf = eglCreatePlatformWindowSurfaceEXT(__eglDpy, <EGL config>, __sampleSurf, NULL);
    __eglCtx  = eglCreateContext(__eglDpy, <EGL config>, EGL_NO_CONTEXT, NULL);

    eglMakeCurrent(__eglDpy, __eglSurf, __eglSurf, __eglCtx);

    /* Rest of the EGL and OpenGL setup */
    [...]

    while (1) {
        sample_dispatch_events(__sampleDpy);

        /* Do render */
        [...]

        eglSwapBuffers(__eglDpy, __eglSurf);
    }

    /* Cleanup */
    [...]
}
