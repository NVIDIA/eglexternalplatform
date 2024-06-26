EGL External Platform Interface
===============================

Overview
--------

This is a work-in-progress specification of the EGL External Platform interface
for writing EGL platforms and their interactions with modern window systems on
top of existing low-level EGL platform implementations. This keeps window system
implementation specifics out of EGL drivers by using application-facing EGL
functions.

Examples of low-level EGL platforms are `EGL_EXT_platform_device` or
`EGL_MESA_platform_surfaceless`.


Installing the interface
------------------------

This is a headers-only specification of the interface.

A `meson.build` file is included, which will install the header files and
generate a matching pkg-config file.

Alternately, `meson.build` has the necessary `override_dependency` call to work
as a Meson subproject.

Definitions
-----------

The following terms are used throughout this README file:

 * *EGL driver*

   An implementation of the full EGL API, either as a vendor library loaded by
   GLVND, or as a standalone library linked against by applications.

 * *EGL platform*

   A rendering system an EGL driver can support at runtime. An EGL platform may
   refer to a window system (e.g. X11, Wayland) or a headless rendering platform
   (e.g. EGLDevice, GBM).

   See section *2.1 "Native Platforms and Rendering APIs"* of the EGL 1.5
   specification, or *EGL_EXT_platform_base* extension.

 * *EGL platform library*

   An implementation of a single EGL External Platform interface on top of
   interfaces provided by an EGL driver.

 * *EGL entrypoint layer*

   Thin layer sitting on top of an EGL driver internal implementation that will
   dispatch calls coming from applications (or GLVND) to either an EGL platform
   library or the EGL driver itself.

 * *EGL External Platform interface*

   Set of functions, hooks, and data structures definitions an EGL entrypoint
   layer will use to interact with EGL platform libraries.

 * *EGL external & internal object handle*

   An external object handle refers to the EGL object handle given to the
   application. These may be provided by either an EGL platform library or the
   EGL driver, depending on what platform the object belongs to.

   In turn, an internal object handle refers to the EGL object handle that only
   the EGL driver internal implementation understands.


Interface walk-through
----------------------

All functions and hooks of an EGL platform library are made available either as
an exports table or dynamically loaded hooks to the EGL entrypoint layer. A
special entry point `loadEGLExternalPlatform()` function must be used to load
all exports and data of a given EGL platform library.

`loadEGLExternalPlatform()` takes *major* and *minor* numbers corresponding to
the version of the EGL External Platform interface the EGL entrypoint layer will
use to interact with the loaded platform. This provides a means for both the
interface and EGL platform libraries to evolve separately in a backwards
compatible way.

Different types of functions and hooks are defined and described below. Unless
otherwise specified, the following functions are made available as an exports
table to the EGL entrypoint layer:

 * *Pure EGL hooks*

   They are intended to be used in replacement of application-facing EGL
   functions. Pure EGL hooks are not provided as entries of the external exports
   table, but are retrieved dynamically with the 'getHookAddress()' export. An
   EGL platform library can provide a hook for most of the application-facing
   functions the EGL entrypoint layer is aware of.

   Examples of these are, among others, hooks for
   `eglCreatePlatformWindowSurface()` or `eglSwapBuffers()`.

 * *Derivatives of EGL functions*

   These are variations of application-facing EGL functions that may require
   extra parameters or will have a sligthly different behavior in order to help
   the EGL entrypoint layer operate in presence of EGL platform libraries.

   An example of these is `queryString()` which is symmetric to
   `eglQueryString()`, but a new EGLExtPlatformString enum is given for the
   string name instead. It helps `eglQueryString()` to return the appropriate
   extension string depending on what EGL platform libraries are available, for
   instance.

 * *External object validation functions*

   The goal of this type of function is to help the EGL entrypoint layer to
   determine which EGL platform library should handle which calls when opaque
   native resources are given.

   An example of these functions is `isValidNativeDisplay()`, which helps
   `eglGetDisplay()`.

 * *External -> Internal object translation functions*

   Whenever non-externally implemented EGL functions are called, translation
   from external (EGL platform library) object handles to internal (EGL driver)
   ones is required.

   `getInternalHandle()` returns the EGL internal handle of a given external
   object.

 * *Callbacks*

   Sometimes, there might be operations that require execution of
   non-application-facing code within the EGL platform library. The EGL External
   Platform interface provides a means for registering callbacks in such cases.

   Unlike the previously described functions, which are implemented by an EGL
   platform library and made available to the EGL entrypoint layer, these
   callbacks allow the latter to register EGL driver functions with the former.

   An example of these is the `eglSetError()` callback that allows EGL platform
   libraries to set EGL error codes to be queried by the application in case of
   failure.

More detailed information of every symbol the EGL External Platform interface
defines can be found in the `interface/eglexternalplatform.h` file.


Interactions with the EGL driver
--------------------------------

Discovery and registration of EGL platform libraries is the EGL entrypoint
layer's responsibility. What discovery method to use is specific to each
implementation, but it is advisable to use something portable and fully
configurable (see JSON-based vendor libraries loader in GLVND).

The initial interaction of an EGL entrypoint layer with an EGL platform library
happens with `loadEGLExternalPlatform()`. This function allows to retrieve both
the exports table and data such as the platform enumeration value. It also
provides a means for the EGL entrypoint layer to pass in an EGL driver imports
structure such that EGL platform libraries can fetch any driver methods they may
require to use.

`loadEGLExternalPlatform()` takes *major* and *minor* numbers corresponding to
the version of the EGL External Platform interface the EGL entrypoint layer will
use. The EGL platform library must then check those numbers against the
interface version it implements, and return the appropriate exports and data
(or fail if versions are not compatible).

EGL platform libraries may initialize their own private platform data
structure at load time to be given to the EGL entrypoint layer. The EGL
entrypoint layer will in turn pass the structure to all export and hook
functions which take another platform's EGLDisplay, or which do not take an
EGLDisplay as input (client extensions).

All EGLDisplay creation operations will be forwarded to the EGL platform library
`getPlatformDisplay()` export. This gives the EGL entrypoint layer the ability
to track which EGLDisplay belongs to which platform in order to dispatch
subsequent functions.

All EGLSurface creation operations will also be forwarded to the appropriate EGL
platform library hooks. They are required to be externally implemented for
applications to be able to present buffers onto a surface.

Any other EGL object creation operation can be also hooked, but the internal
handle must be always returned.

Note that, by design, all object creation operations must be hooked for objects
that are currently required to be externally backed (EGLDisplay and EGLSurface).

Some functions need to be handled by a particular EGL platform library, but
either do not take an EGLDisplay handle or take an EGLDisplay handle that
belongs to a different platform. These functions will require special handling,
which will be defined on a case-by-case basis. For example, `eglGetDisplay()`
uses the `isValidNativeDisplay()` export in order to determine what EGL platform
library to use, and then the `getPlatformDisplay()` export to actually create
the display.

The following diagram illustrates the control flow between an application,
the EGL driver, and two different EGL platform libraries:

    +-------------------------------+
    |                               |
    |         Application           |
    |                               |
    +--------------+----------------+
                   |
                   |
    +--------------|----------------+
    |              |                |       +------------------------+
    |  EGL driver  |                |       |                        |
    |              |                |  +----> EGL platform library A +-----+
    |  +-----------v-------------+  |  |    |                        |     |
    |  |                         |  |  |    +------------------------+     |
    |  |  EGL entrypoint layer   +-----+                                   |
    |  |                         |  |  |    +------------------------+     |
    |  +-----------+-------------+  |  |    |                        |     |
    |              |                |  +----> EGL platform library B +--+  |
    |              |                |       |                        |  |  |
    |  +-----------v-------------+  |       +------------------------+  |  |
    |  |                         |  |                                   |  |
    |  |                         |  |                                   |  |
    |  |                         <--------------------------------------+  |
    |  |   EGL driver internal   |  |                                      |
    |  |                         |  |                                      |
    |  |                         <-----------------------------------------+
    |  |                         |  |
    |  +-------------------------+  |
    |                               |
    +-------------------------------+


Sample code
-----------

In order to illustrate how to use the EGL External Platform interface, a few
files with sample code can be found under the 'samples' folder:

 * 'samples/sample-egl-server.c':

    Sample code for a display server working on top of the EGLStream family of
    extensions.

 * 'samples/sample-egl-client.c':

    Sample code for an EGL application that would run on Foo window system.

 * 'samples/libsample-egl-platform.c':

    Sample code for an EGL External Platform implementation that would add
    EGL_PLATFORM_SAMPLE support on top of EGLStream family of extensions.

Note that these source files are incomplete, and are not intended to be used
as-is.

Also, the NVIDIA Wayland implementation can be found at:

https://github.com/NVIDIA/egl-wayland


Acknowledgements
----------------

Thanks to James Jones for the original implementation of the Wayland external
platform that led to the design of the EGL External Platform infrastructure.


### EGL External Platform interface ###

The EGL External Platform interface itself is licensed as follows:

    Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
