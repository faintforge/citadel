#include "spire.h"
#ifdef SP_OS_EMSCRIPTEN

#include "../../cit_internal.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

typedef struct emsc_gl_surface emsc_gl_surface;
struct emsc_gl_surface {
    EGLSurface surface;
};

typedef struct emsc_gl_state emsc_gl_state;
struct emsc_gl_state {
    EGLDisplay dpy;
    EGLContext ctx;
    EGLConfig config;
};

static emsc_gl_state gl_state = {0};

b8 cit_os_gl_init(cit_config config) {
    sp_assert(config.gl.es, "Emscripten only supports OpenGL ES!");
    sp_assert(config.gl.version_major == 2, "Emscripten only supports OpenGL ES 2.0!");
    sp_assert(config.gl.version_minor == 0, "Emscripten only supports OpenGL ES 2.0!");

    EGLDisplay dpy = eglGetDisplay(NULL);
    i32 major, minor;
    if (!eglInitialize(dpy, &major, &minor)) {
        sp_error("EGL failed to initialize!");
        return false;
    }
    sp_info("EGL version: %d.%d", major, minor);

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        sp_error("Failed to select OpenGL ES API!");
        return false;
    }

    EGLConfig egl_config;
    EGLint num_configs = 0;
    if (!eglChooseConfig(dpy, (i32[]) {
            EGL_CONFORMANT,        EGL_OPENGL_ES_BIT,
            EGL_RENDERABLE_TYPE,   EGL_OPENGL_ES_BIT,
            EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,

            EGL_RED_SIZE,      8,
            EGL_GREEN_SIZE,    8,
            EGL_BLUE_SIZE,     8,
            EGL_ALPHA_SIZE,    EGL_DONT_CARE,
            EGL_DEPTH_SIZE,   24,
            EGL_STENCIL_SIZE,  8,

            EGL_NONE,
        }, &egl_config, 1, &num_configs) || num_configs < 1) {
        sp_error("No suitable EGL config found!");
        return false;
    }

    EGLSurface ctx_surface = eglCreateWindowSurface(dpy,
        egl_config,
        0,
        (const i32[]) {
            // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE,
        });
    if (ctx_surface == EGL_NO_SURFACE) {
        sp_error("EGL failed to create window surface!");
        sp_error("Error: 0x%04X", eglGetError());
        return false;
    }

    i32 ctx_attribs[8] = {
        EGL_CONTEXT_CLIENT_VERSION,  2,
        EGL_NONE,
    };
    EGLContext ctx = eglCreateContext(dpy, egl_config, EGL_NO_CONTEXT, ctx_attribs);
    if (ctx == EGL_NO_CONTEXT) {
        sp_error("Error: 0x%04X", eglGetError());
        return false;
    }

    if (!eglMakeCurrent(dpy, ctx_surface, ctx_surface, ctx)) {
        sp_error("Error: 0x%04X", eglGetError());
        return 0;
    }

    eglDestroySurface(dpy, ctx_surface);

    const GLubyte* (*glGetString)(GLenum) = (const GLubyte* (*)(GLenum)) eglGetProcAddress("glGetString");
    sp_info("Vendor: %s", glGetString(GL_VENDOR));
    sp_info("Renderer: %s", glGetString(GL_RENDERER));
    sp_info("Version: %s", glGetString(GL_VERSION));
    sp_info("Shading language: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    gl_state = (emsc_gl_state) {
        .dpy = dpy,
        .ctx = ctx,
        .config = egl_config,
    };

    return true;
}

void cit_os_gl_terminate(void) {
    eglMakeCurrent(gl_state.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(gl_state.dpy, gl_state.ctx);
    eglTerminate(gl_state.dpy);
}

cit_surface* cit_os_gl_surface_create(cit_window* window) {
    (void) window;
    emsc_gl_surface* surface = sp_arena_push_no_zero(_cit_state.arena, sizeof(emsc_gl_surface));
    surface->surface = eglCreateWindowSurface(gl_state.dpy,
        gl_state.config,
        0,
        (const i32[]) {
            // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE,
        });
    if (surface->surface == EGL_NO_SURFACE) {
        sp_error("EGL failed to create window surface!");
        sp_error("Error: 0x%04X", eglGetError());
        return false;
    }

    return surface;
}

void cit_os_gl_surface_destroy(cit_surface* surface) {
    emsc_gl_surface* emsc_surf = surface;
    eglDestroySurface(gl_state.dpy, emsc_surf->surface);
}

void super_illegal_swap_buffers_function(cit_surface* surface) {
    emsc_gl_surface* emsc_surf = surface;
    eglMakeCurrent(gl_state.dpy, emsc_surf->surface, emsc_surf->surface, gl_state.ctx);
    eglSwapBuffers(gl_state.dpy, emsc_surf->surface);
}

#endif // SP_OS_EMSCRIPTEN
