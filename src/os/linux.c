#include "spire.h"
#ifdef SP_OS_LINUX

#include "linux_internal.h"
#include "../cit_internal.h"
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

cit_linux_state linux_state = {0};

b8 cit_os_init(void) {
    linux_state.xdpy = XOpenDisplay(NULL);
    if (linux_state.xdpy == NULL) {
        return false;
    }

    linux_state.conn = XGetXCBConnection(linux_state.xdpy);
    if (linux_state.conn == NULL) {
        return false;
    }
    XSetEventQueueOwner(linux_state.xdpy, XCBOwnsEventQueue);

    return true;
}

void cit_os_terminate(void) {
    XCloseDisplay(linux_state.xdpy);
}

b8 cit_window_is_open(const cit_window* window) {
    return window->is_open;
}

void cit_poll_events(void) {
    // https://xcb.freedesktop.org/tutorial/events/
    xcb_generic_event_t* ev;
    while ((ev = xcb_poll_for_event(linux_state.conn)) != NULL) {
        switch (ev->response_type & ~0x80) {
            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t* e = (xcb_client_message_event_t *) ev;
                cit_window* window = _cit_state.window_stack;
                linux_window* lwin;
                while (window != NULL) {
                    lwin = window->internal;
                    if (lwin->handle == e->window) {
                        break;
                    }
                    window = window->next;
                }
                if (window == NULL) {
                    sp_error("Unhandled window event.");
                    break;
                }

                if (e->data.data32[0] == lwin->destroy_atom) {
                    window->is_open = false;
                }
            } break;
            default:
                break;
        }
        free(ev);
    }
}

linux_window* internal_linux_window_create(cit_window_desc desc, xcb_visualid_t visual_id) {
    linux_window* lwin = sp_arena_push_no_zero(_cit_state.arena, sizeof(linux_window));

    const xcb_setup_t* setup = xcb_get_setup(linux_state.conn);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;
    if (visual_id == INTERNAL_LINUX_VISUAL_ID_DONT_CARE) {
        visual_id = screen->root_visual;
    }

    u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    u32 values[] = {
        screen->black_pixel,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_EXPOSURE,
    };
    lwin->handle = xcb_generate_id(linux_state.conn);
    xcb_create_window(
            linux_state.conn,
            XCB_COPY_FROM_PARENT,
            lwin->handle,
            screen->root,
            0, 0,
            desc.size.x, desc.size.y,
            0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            visual_id,
            mask, values);

    // Set window properties
    // https://xcb.freedesktop.org/windowcontextandmanipulation/

    // Title
    xcb_change_property(linux_state.conn,
            XCB_PROP_MODE_REPLACE,
            lwin->handle,
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
        xcb_icccm_set_wm_size_hints(linux_state.conn,
                lwin->handle,
                XCB_ATOM_WM_NORMAL_HINTS,
                &hints);
    }

    // Destroy window event
    xcb_intern_atom_cookie_t protocol_cookie = xcb_intern_atom(linux_state.conn, true, 12, "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t destroy_cookie = xcb_intern_atom(linux_state.conn, false, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* protocol_reply = xcb_intern_atom_reply(linux_state.conn, protocol_cookie, NULL);
    xcb_intern_atom_reply_t* destroy_reply = xcb_intern_atom_reply(linux_state.conn, destroy_cookie, NULL);
    xcb_change_property(linux_state.conn,
            XCB_PROP_MODE_REPLACE,
            lwin->handle,
            protocol_reply->atom,
            XCB_ATOM_ATOM,
            32,
            1,
            &destroy_reply->atom);
    lwin->destroy_atom = destroy_reply->atom;

    // Show the window
    xcb_map_window(linux_state.conn, lwin->handle);
    xcb_flush(linux_state.conn);

    return lwin;
}

#endif // SP_OS_LINUX
