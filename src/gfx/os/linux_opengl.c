#include "spire.h"
#ifdef SP_OS_LINUX

#include "../../cit_internal.h"
#include "../../os/linux_internal.h"

#include <EGL/egl.h>
#include <GL/gl.h>

typedef struct cit_gfx_gl_state cit_gfx_gl_state;
struct cit_gfx_gl_state {
    EGLDisplay* dpy;
    EGLContext* ctx;
    EGLConfig config;
};

static cit_gfx_gl_state gl_state = {0};

typedef struct linux_gl_window linux_gl_window;
struct linux_gl_window {
    EGLSurface surface;
};

b8 cit_os_gl_init(cit_config config) {
    EGLDisplay dpy = eglGetDisplay(linux_state.xdpy);
    // EGLDisplay dpy = eglGetPlatformDisplay(EGL_PLATFORM_XCB_EXT, linux_state.conn, (const EGLAttrib[]) {
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

void cit_os_gl_terminate(void) {
    eglMakeCurrent(gl_state.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(gl_state.dpy, gl_state.ctx);
    eglTerminate(gl_state.dpy);
}

cit_window* cit_os_gl_window_create(cit_window_desc desc) {
    i32 visual_id;
    if (!eglGetConfigAttrib(gl_state.dpy, gl_state.config, EGL_NATIVE_VISUAL_ID, &visual_id)) {
        sp_error("EGL failed to get native visual id!");
        return NULL;
    }

    cit_window* win = sp_arena_push_no_zero(_cit_state.arena, sizeof(cit_window));
    linux_gl_window* lglwin = sp_arena_push_no_zero(_cit_state.arena, sizeof(linux_gl_window));
    linux_window* lwin = internal_linux_window_create(desc, visual_id);
    lwin->internal = lglwin;
    *win = (cit_window) {
        .internal = lwin,
        .is_open = true,
    };

    EGLSurface surface = eglCreateWindowSurface(gl_state.dpy,
        gl_state.config,
        (EGLNativeWindowType) lwin->handle,
        (const i32[]) {
            // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE,
        });
    if (surface == EGL_NO_SURFACE) {
        sp_error("EGL failed to create window surface!");
        sp_debug("code: %llu", surface);
        return false;
    }
    lglwin->surface = surface;

    eglMakeCurrent(gl_state.dpy, surface, surface, gl_state.ctx);

    return win;
}

void cit_os_gl_window_destroy(cit_window *window) {
    linux_window* lwin = window->internal;
    linux_gl_window* lglwin = (linux_gl_window*) lwin->internal;
    xcb_destroy_window(linux_state.conn, lwin->handle);
    eglDestroySurface(gl_state.dpy, lglwin->surface);
}

#endif // SP_OS_LINUX
