#include "spire.h"
#ifdef SP_OS_LINUX

#include "../gfx_internal.h"
#include "../../os/linux/internal.h"

#include <EGL/egl.h>
#include <GL/gl.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

typedef struct cit_gfx_gl_state cit_gfx_gl_state;
struct cit_gfx_gl_state {
    EGLDisplay* dpy;
    EGLContext* ctx;
    EGLConfig config;
};

static cit_gfx_gl_state gl_state = {0};

struct cit_window {
    xcb_window_t handle;
    xcb_atom_t destroy_atom;
    b8 is_open;
};

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

void cit_gfx_os_opengl_terminate(void) {
    eglMakeCurrent(gl_state.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(gl_state.dpy, gl_state.ctx);
    eglTerminate(gl_state.dpy);
}

cit_window* cit_gfx_os_opengl_window_create(cit_window_desc desc) {
    cit_window* window = sp_arena_push_no_zero(os_state.arena, sizeof(cit_window));
    window->is_open = true;

    i32 visual_id;
    eglGetConfigAttrib(gl_state.dpy, gl_state.config, EGL_NATIVE_VISUAL_ID, &visual_id);
    const xcb_setup_t* setup = xcb_get_setup(os_state.conn);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

    u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    u32 values[] = {
        screen->black_pixel,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_EXPOSURE,
    };
    window->handle = xcb_generate_id(os_state.conn);
    xcb_create_window(
            os_state.conn,
            XCB_COPY_FROM_PARENT,
            window->handle,
            screen->root,
            0, 0,
            desc.size.x, desc.size.y,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            visual_id,
            mask, values);

    // xcb_change_window_attributes(os_state.conn, window->handle, XCB_CW_COLORMAP, &colormap);

    // Set window properties
    // https://xcb.freedesktop.org/windowcontextandmanipulation/

    // Title
    xcb_change_property(os_state.conn,
            XCB_PROP_MODE_REPLACE,
            window->handle,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            desc.title.len,
            desc.title.data);

    // Set resizability
    if (!desc.resizable) {
        xcb_size_hints_t hints;
        xcb_icccm_size_hints_set_min_size(&hints, desc.size.x, desc.size.y);
        xcb_icccm_size_hints_set_max_size(&hints, desc.size.x, desc.size.y);
        xcb_icccm_set_wm_size_hints(os_state.conn,
                window->handle,
                XCB_ATOM_WM_NORMAL_HINTS,
                &hints);
    }

    // Destroy window event
    xcb_intern_atom_cookie_t protocol_cookie = xcb_intern_atom(os_state.conn, true, 12, "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t destroy_cookie = xcb_intern_atom(os_state.conn, false, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* protocol_reply = xcb_intern_atom_reply(os_state.conn, protocol_cookie, NULL);
    xcb_intern_atom_reply_t* destroy_reply = xcb_intern_atom_reply(os_state.conn, destroy_cookie, NULL);
    xcb_change_property(os_state.conn,
            XCB_PROP_MODE_REPLACE,
            window->handle,
            protocol_reply->atom,
            XCB_ATOM_ATOM,
            32,
            1,
            &destroy_reply->atom);
    window->destroy_atom = destroy_reply->atom;

    // Show the window
    xcb_map_window(os_state.conn, window->handle);
    xcb_flush(os_state.conn);

    EGLSurface surface = eglCreateWindowSurface(gl_state.dpy,
        gl_state.config,
        (EGLNativeWindowType) window->handle,
        (const i32[]) {
            // EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_LINEAR,
            EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
            EGL_NONE,
        });
    if (surface == EGL_NO_SURFACE) {
        sp_error("EGL failed to create window surface!");
        sp_error("Code: %llu", surface);
        return false;
    }

    return window;
}

cit_gfx_interface cit_gfx_opengl_interface = {
    .init = cit_gfx_os_opengl_init,
    .terminate = cit_gfx_os_opengl_terminate,
    .window_create = cit_gfx_os_opengl_window_create,
};

#endif // SP_OS_LINUX
