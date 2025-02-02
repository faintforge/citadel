#include "spire.h"
#ifdef SP_OS_LINUX

#include "internal.h"
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include "../os_internal.h"

xcb_visualtype_t* find_xcb_visual(xcb_screen_t* screen) {
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);

        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
            if (visual_iter.data->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
                return visual_iter.data;
            }
        }
    }

    return NULL;
}

cit_window* cit_window_create(SP_Ivec2 size, SP_Str title, b8 resizable) {
    cit_window* window = sp_arena_push_no_zero(os_state.arena, sizeof(cit_window));
    window->is_open = true;

    const xcb_setup_t* setup = xcb_get_setup(os_state.conn);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;
    xcb_visualtype_t* visual = find_xcb_visual(screen);
    window->visual_id = visual->visual_id;

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
            size.x, size.y,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            visual->visual_id,
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
            title.len,
            title.data);

    // Set resizability
    if (!resizable) {
        xcb_size_hints_t hints;
        xcb_icccm_size_hints_set_min_size(&hints, size.x, size.y);
        xcb_icccm_size_hints_set_max_size(&hints, size.x, size.y);
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

    window->next = os_state.window_stack;
    os_state.window_stack = window;

    // TODO: REMOVE
    cit_os_gfx_gl_interface.equip_window(window);

    return window;
}

void cit_window_destroy(cit_window* window) {
    xcb_destroy_window(os_state.conn, window->handle);
}

b8 cit_window_is_open(const cit_window* window) {
    return window->is_open;
}

void cit_poll_events(void) {
    // https://xcb.freedesktop.org/tutorial/events/
    xcb_generic_event_t* ev;
    while ((ev = xcb_poll_for_event(os_state.conn)) != NULL) {
        switch (ev->response_type & ~0x80) {
            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t* e = (xcb_client_message_event_t *) ev;
                cit_window* window = os_state.window_stack;
                while (window != NULL) {
                    if (window->handle == e->window) {
                        break;
                    }
                    window = window->next;
                }
                if (window == NULL) {
                    sp_error("Unhandled window event.");
                    break;
                }

                if (e->data.data32[0] == window->destroy_atom) {
                    window->is_open = false;
                }
            } break;
            default:
                break;
        }
        free(ev);
    }
}

#endif // SP_OS_LINUX
