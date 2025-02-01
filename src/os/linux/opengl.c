#include "spire.h"
#ifdef SP_OS_LINUX

#include "../os_internal.h"

#include <EGL/egl.h>
#include <GL/gl.h>
#include <GL/glext.h>

b8 cit_os_gfx_gl_init(void) {
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY) {
        sp_error("No EGL display found!");
        return false;
    }

    i32 egl_major;
    i32 egl_minor;
    if (!eglInitialize(dpy, &egl_major, &egl_minor)) {
        sp_error("EGL failed to initialize!");
        return false;
    }

    eglBindAPI(EGL_OPENGL_API);

    EGLint num_configs = 0;
    EGLConfig config;
    if (!eglChooseConfig(dpy, (i32[]) {
            EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
            EGL_CONFORMANT,        EGL_OPENGL_BIT,
            EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
            EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,

            EGL_RED_SIZE,      8,
            EGL_GREEN_SIZE,    8,
            EGL_BLUE_SIZE,     8,
            EGL_DEPTH_SIZE,   24,
            EGL_STENCIL_SIZE,  8,

            EGL_NONE,
        }, &config, 1, &num_configs) || num_configs < 1) {
        sp_error("No suitable EGL config found!");
        return false;
    }

    EGLContext ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, (i32[]) {
            EGL_CONTEXT_MAJOR_VERSION, 4,
            EGL_CONTEXT_MINOR_VERSION, 6,
            EGL_NONE,
        });
    if (ctx == EGL_NO_CONTEXT) {
        sp_error("EGL failed to create an OpenGL context!");
        return false;
    }

    if (!eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx)) {
        sp_error("EGL failed to make context current!");
        return false;
    }

    const GLubyte* (*glGetString)(GLenum) = (const GLubyte* (*)(GLenum)) eglGetProcAddress("glGetString");
    sp_info("Vendor: %s", glGetString(GL_VENDOR));
    sp_info("Renderer: %s", glGetString(GL_RENDERER));
    sp_info("Version: %s", glGetString(GL_VERSION));
    sp_info("Shading language: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    return true;
}

void cit_os_gfx_gl_terminate(void) { }

cit_os_gfx_interface cit_os_gfx_gl_interface = {
    .init      = cit_os_gfx_gl_init,
    .terminate = cit_os_gfx_gl_terminate,
};

#endif // SP_OS_LINUX
