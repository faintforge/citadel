#include "citadel.h"
#include "spire.h"
#ifdef SP_OS_LINUX

#include "../os_internal.h"
#include "internal.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <GL/glext.h>

typedef struct cit_os_gfx_gl_window_data cit_os_gfx_gl_window_data;
struct cit_os_gfx_gl_window_data {
    EGLSurface surface;
};

typedef struct cit_os_gfx_gl_state cit_os_gfx_gl_state;
struct cit_os_gfx_gl_state {
    SP_Arena* arena;
    EGLDisplay dpy;
    EGLContext ctx;
    EGLConfig config;
};

static cit_os_gfx_gl_state gfx_gl_state = {0};

b8 cit_os_gfx_gl_init(void) {
    EGLDisplay dpy = eglGetDisplay(os_state.xdpy);
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

    if (!eglBindAPI(EGL_OPENGL_API)) {
        sp_error("Failed to select OpenGL API!");
        return false;
    }

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
        }, &gfx_gl_state.config, 1, &num_configs) || num_configs < 1) {
        sp_error("No suitable EGL config found!");
        return false;
    }

    EGLContext ctx = eglCreateContext(dpy, gfx_gl_state.config, EGL_NO_CONTEXT, (i32[]) {
            EGL_CONTEXT_MAJOR_VERSION, 4,
            EGL_CONTEXT_MINOR_VERSION, 6,
            EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
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

    gfx_gl_state = (cit_os_gfx_gl_state) {
        .arena = sp_arena_create(),
        .dpy = dpy,
        .ctx = ctx,
    };
    sp_arena_tag(gfx_gl_state.arena, sp_str_lit("graphics"));

    return true;
}

void cit_os_gfx_gl_terminate(void) {
    eglMakeCurrent(gfx_gl_state.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(gfx_gl_state.dpy, gfx_gl_state.ctx);
    eglTerminate(gfx_gl_state.dpy);
    sp_arena_destroy(gfx_gl_state.arena);
}

void cit_os_gfx_gl_equip_window(cit_window* window) {
    // EGLSurface surface = eglCreateWindowSurface(gfx_gl_state.dpy,
    //     gfx_gl_state.config,
    //     (EGLNativeWindowType) window->handle,
    //     (const i32[]) {
    //         // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
    //         EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
    //         EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
    //         EGL_NONE,
    //     });
    EGLSurface surface = eglCreateWindowSurface(gfx_gl_state.dpy,
        gfx_gl_state.config,
        (EGLNativeWindowType) window->handle,
        NULL);
    if (surface == EGL_NO_SURFACE) {
        sp_error("EGL failed to create window surface!");
        sp_error("Code: %llu", surface);
        return;
    }

    eglMakeCurrent(gfx_gl_state.dpy, surface, surface, gfx_gl_state.ctx);
    eglSwapBuffers(gfx_gl_state.dpy, surface);
    // glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);

    cit_os_gfx_gl_window_data* data = sp_arena_push_no_zero(gfx_gl_state.arena, sizeof(cit_os_gfx_gl_window_data));
    data->surface = surface;
    window->gfx_data = data;
}

cit_os_gfx_interface cit_os_gfx_gl_interface = {
    .init         = cit_os_gfx_gl_init,
    .terminate    = cit_os_gfx_gl_terminate,
    .equip_window = cit_os_gfx_gl_equip_window,
};

#endif // SP_OS_LINUX
