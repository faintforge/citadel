#include "spire.h"
#ifdef SP_OS_LINUX

#include "../gfx_internal.h"
#include "../../os/linux/internal.h"

#include <EGL/egl.h>
#include <GL/gl.h>

typedef struct cit_gfx_gl_state cit_gfx_gl_state;
struct cit_gfx_gl_state {
    EGLDisplay* dpy;
    EGLContext* ctx;
    EGLConfig config;
};

static cit_gfx_gl_state gl_state = {0};

b8 cit_gfx_os_opengl_init(cit_config config) {
    EGLDisplay dpy = eglGetDisplay(os_state.xdpy);
    // EGLDisplay dpy = eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, os_state.conn, (const EGLAttrib[]) {
    //         EGL_PLATFORM_XCB_SCREEN_EXT,
    //         0,
    //         EGL_NONE,
    //     });
    // EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
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
    sp_debug("EGL version: %d.%d", egl_major, egl_minor);

    if (config.gl.es) {
        if (!eglBindAPI(EGL_OPENGL_ES_API)) {
            sp_error("Failed to select OpenGL ES API!");
            return false;
        }
    } else {
        if (!eglBindAPI(EGL_OPENGL_API)) {
            sp_error("Failed to select OpenGL API!");
            return false;
        }
    }

    EGLConfig egl_config;
    EGLint num_configs = 0;
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
        }, &egl_config, 1, &num_configs) || num_configs < 1) {
        sp_error("No suitable EGL config found!");
        return false;
    }

    i32 ctx_attribs[8] = {
        EGL_CONTEXT_MAJOR_VERSION, config.gl.version_major,
        EGL_CONTEXT_MINOR_VERSION, config.gl.version_minor,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE,
    };
    // Replace EGL_CONTEXT_OPENGL_PROFILE_MASK with EGL_NONE to not set a
    // profile mask for ES contexts.
    if (config.gl.es) {
        ctx_attribs[4] = EGL_NONE;
    }

    EGLContext ctx = eglCreateContext(dpy, egl_config, EGL_NO_CONTEXT, ctx_attribs);

    if (ctx == EGL_NO_CONTEXT) {
        sp_error("EGL failed to create an OpenGL context!");
        return false;
    }

    if (!eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx)) {
        sp_error("EGL failed to make context current!");
        return false;
    }

    gl_state = (cit_gfx_gl_state) {
        .dpy = dpy,
        .ctx = ctx,
        .config = egl_config,
    };

    const GLubyte* (*glGetString)(GLenum) = (const GLubyte* (*)(GLenum)) eglGetProcAddress("glGetString");
    sp_info("Vendor: %s", glGetString(GL_VENDOR));
    sp_info("Renderer: %s", glGetString(GL_RENDERER));
    sp_info("Version: %s", glGetString(GL_VERSION));
    sp_info("Shading language: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    return true;
}

void cit_gfx_os_opengl_terminate(void) { }

cit_gfx_interface cit_gfx_opengl_interface = {
    .init = cit_gfx_os_opengl_init,
    .terminate = cit_gfx_os_opengl_terminate,
};

#endif // SP_OS_LINUX
