#include "spire.h"
#ifdef SP_OS_LINUX

#include "linux_internal.h"
#include "../cit_internal.h"
#include "citadel.h"

#include <locale.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xproto.h>
#include <xcb/xkb.h>
#include <xcb/xcb_keysyms.h>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>

cit_linux_state linux_state = {0};

b8 cit_os_init(void) {
    setlocale(LC_CTYPE, "");
    linux_state.xdpy = XOpenDisplay(NULL);
    if (linux_state.xdpy == NULL) {
        return false;
    }

    linux_state.conn = XGetXCBConnection(linux_state.xdpy);
    if (linux_state.conn == NULL) {
        return false;
    }
    XSetEventQueueOwner(linux_state.xdpy, XCBOwnsEventQueue);

    XSetLocaleModifiers("");
    linux_state.xim = XOpenIM(linux_state.xdpy, NULL, NULL, NULL);
    if (linux_state.xim == NULL) {
        return false;
    }

    return true;
}

void cit_os_terminate(void) {
    XCloseIM(linux_state.xim);
    XCloseDisplay(linux_state.xdpy);
}

b8 cit_window_is_open(const cit_window* window) {
    return window->is_open;
}

static cit_window* get_window_from_event(xcb_window_t event_window) {
    cit_window* window = _cit_state.window_stack;
    linux_window* lwin;
    while (window != NULL) {
        lwin = window->internal;
        if (lwin->handle == event_window) {
            return window;
        }
        window = window->next;
    }
    return NULL;
}

static u32 utf8_to_utf32(char* buffer) {
    // Get the amount of bytes from the first byte of the utf8 encoded string.
    u8 c = buffer[0];
    u8 shift_width = 7;
    while (((c >> shift_width) & 1) != 0) {
        shift_width--;
    }
    u8 byte_count = 7 - shift_width;

    // ASCII
    if (byte_count == 0) {
        return buffer[0];
    }

    // Unicode
    // https://en.wikipedia.org/wiki/UTF-8#Description
    u8 first_byte_mask[3] = {
        0b00011111, // 2 bytes
        0b00001111, // 3 bytes
        0b00000111, // 4 bytes
    };
    u32 codepoint = 0;
    u8 first_byte_bits = buffer[0] & first_byte_mask[byte_count - 2];
    codepoint |= first_byte_bits << 6 * (byte_count - 1);
    for (i8 i = byte_count - 1; i > 0; i--) {
        codepoint |= (u32) (buffer[i] & 0b00111111) << (6 * (i - 1));
    }

    return codepoint;
}

cit_event handle_key_event(linux_window* lwin, XKeyEvent* ev) {
    cit_event cit_ev = {0};

    // Translate event
    char buffer[32];
    KeySym keysym;
    Status status;
    i32 len = Xutf8LookupString(lwin->xic, ev, buffer, sizeof(buffer) - 1, &keysym, &status);
    if (status == XBufferOverflow) {
        sp_warn("Buffer too small for character input!");
        return cit_ev;
    } else if ((status == XLookupChars || status == XLookupBoth) && len > 0) {
        cit_ev.key.codepoint = utf8_to_utf32(buffer);
        // sp_debug("codepoint: %u, '%lc'", cit_ev.key.codepoint, cit_ev.key.codepoint);
    } else if (status == XLookupNone) {
        return cit_ev;
    }

    // Event type
    if (ev->type == KeyPress)   { cit_ev.type = CIT_EVENT_TYPE_KEY_PRESS; }
    if (ev->type == KeyRelease) { cit_ev.type = CIT_EVENT_TYPE_KEY_RELEASE; }

    // Scandcode
    cit_ev.key.scancode = ev->keycode;

    // Mod
    cit_mod mod = CIT_MOD_NONE;
    if (ev->state & XCB_MOD_MASK_SHIFT)   { mod |= CIT_MOD_SHIFT; }
    if (ev->state & XCB_MOD_MASK_CONTROL) { mod |= CIT_MOD_CRTL; }
    if (ev->state & XCB_MOD_MASK_1)       { mod |= CIT_MOD_ALT_L; }
    if (ev->state & XCB_MOD_MASK_5)       { mod |= CIT_MOD_ALT_R; }
    cit_ev.key.mod = mod;

    return cit_ev;
}

cit_event* cit_poll_events(void) {
    sp_arena_clear(_cit_state.events_arena);
    cit_event* first_event = NULL;
    cit_event* last_event = NULL;

    // https://xcb.freedesktop.org/tutorial/events/
    xcb_generic_event_t* ev;
    while ((ev = xcb_poll_for_event(linux_state.conn)) != NULL) {
        switch (ev->response_type & ~0x80) {
            case XCB_CLIENT_MESSAGE: {
                xcb_client_message_event_t* e = (xcb_client_message_event_t*) ev;
                cit_window* window = get_window_from_event(e->window);
                linux_window* lwin = window->internal;
                if (e->data.data32[0] == lwin->destroy_atom) {
                    window->is_open = false;
                }
            } break;

            case XCB_KEY_RELEASE:
            case XCB_KEY_PRESS: {
                xcb_key_press_event_t* e = (xcb_key_press_event_t*) ev;
                linux_window* lwin = get_window_from_event(e->event)->internal;

                XKeyEvent xkey = {
                    .type        = KeyPress,
                    .display     = linux_state.xdpy,
                    .window      = lwin->handle,
                    .root        = e->root,
                    .subwindow   = None,
                    .time        = e->time,
                    .x           = e->event_x,
                    .y           = e->event_y,
                    .x_root      = e->root_x,
                    .y_root      = e->root_y,
                    .state       = e->state,
                    .keycode     = e->detail,
                    .same_screen = e->same_screen,
                };

                if (XFilterEvent((XEvent*) &xkey, xkey.window)) {
                    break;
                }

                cit_event cit_ev = handle_key_event(lwin, &xkey);
                if (cit_ev.type == CIT_EVENT_TYPE_NONE) {
                    break;
                }
                // Push event
                cit_event* user_ev = sp_arena_push_no_zero(_cit_state.events_arena, sizeof(cit_event));
                *user_ev = cit_ev;
                if (first_event == NULL) {
                    first_event = user_ev;
                    last_event = user_ev;
                } else {
                    last_event->next = user_ev;
                    last_event = user_ev;
                }
            } break;

            default:
                break;
        }
        free(ev);
    }

    while (XPending(linux_state.xdpy)) {
        XEvent ev;
        XNextEvent(linux_state.xdpy, &ev);
        if (ev.type == KeyPress) {
            XKeyEvent* e = (XKeyEvent*) &ev;
            if (XFilterEvent((XEvent*) e, e->window)) {
                continue;
            }

            linux_window* lwin = get_window_from_event(e->window)->internal;
            cit_event cit_ev = handle_key_event(lwin, e);
            if (cit_ev.type == CIT_EVENT_TYPE_NONE) {
                continue;
            }
            // Push event
            cit_event* user_ev = sp_arena_push_no_zero(_cit_state.events_arena, sizeof(cit_event));
            *user_ev = cit_ev;
            if (first_event == NULL) {
                first_event = user_ev;
                last_event = user_ev;
            } else {
                last_event->next = user_ev;
                last_event = user_ev;
            }
        }
    }

    // Make sure event queue is empty before next polling happens. This
    // prevents latency.
    xcb_flush(linux_state.conn);
    XFlush(linux_state.xdpy);

    return first_event;
}

linux_window* internal_linux_window_create(cit_window_desc desc, xcb_visualid_t visual_id) {
    linux_window* lwin = sp_arena_push_no_zero(_cit_state.arena, sizeof(linux_window));

    const xcb_setup_t* setup = xcb_get_setup(linux_state.conn);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;
    if (visual_id == INTERNAL_LINUX_VISUAL_ID_DONT_CARE) {
        visual_id = screen->root_visual;
    }

    u32 mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    u32 events = XCB_EVENT_MASK_STRUCTURE_NOTIFY |
            XCB_EVENT_MASK_EXPOSURE |
            XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_KEY_RELEASE;
    u32 values[] = {
        screen->black_pixel,
        events,
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
    if (lwin->handle == XCB_WINDOW_NONE) {
        return NULL;
    }

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

    // Create input context
    lwin->xic = XCreateIC(linux_state.xim,
            XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
            XNClientWindow, lwin->handle,
            XNFocusWindow, lwin->handle,
            NULL);
    if (lwin->xic == NULL) {
        return NULL;
    }
    XSetICFocus(lwin->xic);

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

void internal_linux_window_destroy(linux_window* window) {
    XDestroyIC(window->xic);
    xcb_destroy_window(linux_state.conn, window->handle);
}

#endif // SP_OS_LINUX
