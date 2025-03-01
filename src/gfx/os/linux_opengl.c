#include "spire.h"
#ifdef SP_OS_LINUX

#include "../../cit_internal.h"
#include "../../os/linux_internal.h"

#include <EGL/egl.h>
#include <GL/gl.h>

typedef struct cit_gfx_gl_state cit_gfx_gl_state;
struct cit_gfx_gl_state {
    EGLDisplay* dpy;
    EGLContext ctx;
    EGLConfig config;
    xcb_window_t ctx_window;
};

static cit_gfx_gl_state gl_state = {0};

typedef struct linux_gl_surface linux_gl_surface;
struct linux_gl_surface {
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
        sp_error("EGL error: 0x%04X", eglGetError());
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

    EGLConfig egl_configs[64];
    EGLint num_configs = 0;
    if (!eglChooseConfig(dpy, (i32[]) {
            EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
            EGL_CONFORMANT,        EGL_OPENGL_BIT,
            EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
            EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,

            EGL_RED_SIZE,      8,
            EGL_GREEN_SIZE,    8,
            EGL_BLUE_SIZE,     8,
            EGL_ALPHA_SIZE,    EGL_DONT_CARE,
            EGL_DEPTH_SIZE,   24,
            EGL_STENCIL_SIZE,  8,

            EGL_NONE,
        }, egl_configs, sp_arrlen(egl_configs), &num_configs) || num_configs < 1) {
        sp_error("No suitable EGL config found!");
        return false;
    }

    sp_debug("Config count: %d", num_configs);
    EGLConfig egl_config = NULL;
    for (i32 i = 0; i < num_configs; i++) {
        EGLConfig curr_config = egl_configs[i];
        i32 egl_visual_id;
        if (eglGetConfigAttrib(dpy, curr_config, EGL_NATIVE_VISUAL_ID, &egl_visual_id)) {
            if ((xcb_visualid_t) egl_visual_id == linux_state.screen->root_visual) {
                egl_config = curr_config;
                break;
            }
        }
    }

    if (egl_config == NULL) {
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

    // Create a hidden window and a surface because calling 'eglMakeCurrent'
    // with EGL_NO_SURFACE breaks the application, so this is needed to load the
    // OpenGL functions.
    xcb_window_t window = xcb_generate_id(linux_state.conn);
    xcb_create_window(linux_state.conn,
            XCB_COPY_FROM_PARENT,
            window,
            linux_state.screen->root,
            0, 0,
            1, 1,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            linux_state.screen->root_visual,
            XCB_CW_BACK_PIXEL, (const i32[]) {
                linux_state.screen->black_pixel,
            });
    xcb_flush(linux_state.conn);

    EGLSurface ctx_surface = eglCreateWindowSurface(dpy,
        egl_config,
        (EGLNativeWindowType) window,
        (const i32[]) {
            // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE,
        });
    if (ctx_surface == EGL_NO_SURFACE) {
        sp_error("EGL failed to create window surface!");
        sp_debug("Error: 0x%04X", eglGetError());
        return false;
    }


    if (!eglMakeCurrent(dpy, ctx_surface, ctx_surface, ctx)) {
        sp_error("EGL failed to make context current!");
        return false;
    }

    // Since the surface only use is to initialize the OpenGL context we can
    // destroy it right after making the context current.
    xcb_destroy_window(linux_state.conn, window);
    eglDestroySurface(dpy, ctx_surface);

    const GLubyte* (*glGetString)(GLenum) = (const GLubyte* (*)(GLenum)) eglGetProcAddress("glGetString");
    sp_info("Vendor: %s", glGetString(GL_VENDOR));
    sp_info("Renderer: %s", glGetString(GL_RENDERER));
    sp_info("Version: %s", glGetString(GL_VERSION));
    sp_info("Shading language: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    gl_state = (cit_gfx_gl_state) {
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
    linux_gl_surface* surface = sp_arena_push_no_zero(_cit_state.arena, sizeof(linux_gl_surface));

    EGLSurface egl_surface = eglCreateWindowSurface(gl_state.dpy,
        gl_state.config,
        (EGLNativeWindowType) window->handle,
        (const i32[]) {
            // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE,
        });

    if (egl_surface == EGL_NO_SURFACE) {
        sp_error("EGL failed to create window surface!");
        sp_debug("code: %llu", surface);
        return false;
    }
    surface->surface = egl_surface;

    if (!eglMakeCurrent(gl_state.dpy, egl_surface, egl_surface, gl_state.ctx)) {
        sp_error("EGL failed to make surface current!");
        return false;
    }

    return (cit_surface*) surface;
}

void cit_os_gl_surface_destroy(cit_surface* surface) {
    eglDestroySurface(gl_state.dpy, ((linux_gl_surface*) surface)->surface);
}

void super_illegal_swap_buffers_function(cit_surface* surface) {
    linux_gl_surface* lsurf = surface;
    eglSwapInterval(gl_state.dpy, 0);
    eglMakeCurrent(gl_state.dpy, lsurf->surface, lsurf->surface, gl_state.ctx);
    eglSwapBuffers(gl_state.dpy, lsurf->surface);
}

#endif // SP_OS_LINUX
