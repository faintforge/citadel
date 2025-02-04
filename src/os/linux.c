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

static cit_event handle_raw_key_event(cit_window* win, XKeyEvent* ev) {
    cit_event cit_ev = {
        .window = win,
    };

    // TODO: Translate keysym
    KeySym sym = XLookupKeysym(ev, 0);

    // Event type
    if (ev->type == KeyPress)   { cit_ev.type = CIT_EVENT_TYPE_KEY_PRESS; }
    if (ev->type == KeyRelease) { cit_ev.type = CIT_EVENT_TYPE_KEY_RELEASE; }

    // Scandcode
    cit_ev.scancode = ev->keycode;

    // Mod
    cit_mod mod = CIT_MOD_NONE;
    if (ev->state & XCB_MOD_MASK_SHIFT)   { mod |= CIT_MOD_SHIFT; }
    if (ev->state & XCB_MOD_MASK_CONTROL) { mod |= CIT_MOD_CRTL; }
    if (ev->state & XCB_MOD_MASK_1)       { mod |= CIT_MOD_ALT_L; }
    if (ev->state & XCB_MOD_MASK_5)       { mod |= CIT_MOD_ALT_R; }
    cit_ev.mod = mod;

    return cit_ev;
}

static cit_event handle_text_event(cit_window* win, XKeyEvent* ev) {
    linux_window* lwin = win->internal;
    cit_event cit_ev = {
        .window = win,
        .scancode = ev->keycode,
    };

    // Translate event
    char buffer[32];
    KeySym keysym;
    Status status;
    i32 len = Xutf8LookupString(lwin->xic, ev, buffer, sizeof(buffer) - 1, &keysym, &status);
    if (status == XBufferOverflow) {
        sp_warn("Buffer too small for character input!");
        return cit_ev;
    } else if ((status == XLookupChars || status == XLookupBoth) && len > 0) {
        cit_ev.codepoint = utf8_to_utf32(buffer);
    } else if (status == XLookupNone) {
        return cit_ev;
    }

    // TODO: Translate keysym

    // Event type
    // Set event down here because we want to return CIT_EVENT_TYPE_NONE if
    // Xutf8LookupString fails.
    cit_ev.type = CIT_EVENT_TYPE_TEXT;

    // Mod
    cit_mod mod = CIT_MOD_NONE;
    if (ev->state & XCB_MOD_MASK_SHIFT)   { mod |= CIT_MOD_SHIFT; }
    if (ev->state & XCB_MOD_MASK_CONTROL) { mod |= CIT_MOD_CRTL; }
    if (ev->state & XCB_MOD_MASK_1)       { mod |= CIT_MOD_ALT_L; }
    if (ev->state & XCB_MOD_MASK_5)       { mod |= CIT_MOD_ALT_R; }
    cit_ev.mod = mod;

    return cit_ev;
}

static void push_event(cit_event** first, cit_event** last, cit_event event) {
        if (event.type == CIT_EVENT_TYPE_NONE) {
            return;
        }
        cit_event* user_ev = sp_arena_push_no_zero(_cit_state.events_arena, sizeof(cit_event));
        *user_ev = event;
        if (*first == NULL) {
            *first = user_ev;
            *last = user_ev;
        } else {
            user_ev->prev = *last;
            (*last)->next = user_ev;
            *last = user_ev;
        }
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
                    push_event(&first_event, &last_event, (cit_event) {
                            .type = CIT_EVENT_TYPE_WINDOW_CLOSE,
                            .window = window,
                        });
                }
            } break;

            case XCB_CONFIGURE_NOTIFY: {
                xcb_configure_notify_event_t* e = (xcb_configure_notify_event_t*) ev;
                cit_window* window = get_window_from_event(e->window);
                if (window->size.x != e->width || window->size.y != e->height) {
                    window->size = sp_iv2(e->width, e->height);
                    push_event(&first_event, &last_event, (cit_event) {
                            .type = CIT_EVENT_TYPE_WINDOW_RESIZE,
                            .window = window,
                            .size = window->size,
                            });
                }
            } break;

            case XCB_KEY_RELEASE:
            case XCB_KEY_PRESS: {
                xcb_key_press_event_t* e = (xcb_key_press_event_t*) ev;
                cit_window* win = get_window_from_event(e->event);
                linux_window* lwin = win->internal;

                XKeyEvent xkey = {
                    .type        = e->response_type == XCB_KEY_PRESS ? KeyPress : KeyRelease,
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

                // Handle raw input first
                push_event(&first_event, &last_event, handle_raw_key_event(win, &xkey));

                // Filter out textual input
                if (XFilterEvent((XEvent*) &xkey, xkey.window)) {
                    break;
                }

                // Handle text input
                push_event(&first_event, &last_event, handle_text_event(win, &xkey));
            } break;

            case XCB_BUTTON_RELEASE:
            case XCB_BUTTON_PRESS: {
                xcb_button_press_event_t* e = (xcb_button_press_event_t*) ev;

                cit_mouse_button btn = -1;
                i8 scroll = 0;
                switch (e->detail) {
                    case XCB_BUTTON_INDEX_1:
                        btn = CIT_MOUSE_BUTTON_LEFT;
                        break;
                    case XCB_BUTTON_INDEX_2:
                        btn = CIT_MOUSE_BUTTON_MIDDLE;
                        break;
                    case XCB_BUTTON_INDEX_3:
                        btn = CIT_MOUSE_BUTTON_RIGHT;
                        break;
                    case XCB_BUTTON_INDEX_4:
                        scroll = 1;
                        break;
                    case XCB_BUTTON_INDEX_5:
                        scroll = -1;
                        break;
                    default:
                        break;
                }
                if (btn == -1 && scroll == 0) {
                    break;
                }

                cit_window* win = get_window_from_event(e->event);
                if (scroll != 0) {
                    push_event(&first_event, &last_event, (cit_event) {
                            .type = CIT_EVENT_TYPE_MOUSE_SCROLL,
                            .window = win,
                            .position = sp_iv2(e->event_x, e->event_y),
                            .scroll = scroll,
                        });
                    break;
                }

                push_event(&first_event, &last_event, (cit_event) {
                        .type = e->response_type == XCB_BUTTON_PRESS ?
                            CIT_EVENT_TYPE_MOUSE_BUTTON_PRESS :
                            CIT_EVENT_TYPE_MOUSE_BUTTON_RELEASE,
                        .window = win,
                        .position = sp_iv2(e->event_x, e->event_y),
                        .button = btn,
                    });
            } break;

            case XCB_MOTION_NOTIFY: {
                xcb_motion_notify_event_t* e = (xcb_motion_notify_event_t*) ev;
                cit_window* win = get_window_from_event(e->event);
                push_event(&first_event, &last_event, (cit_event) {
                        .type = CIT_EVENT_TYPE_MOUSE_MOVE,
                        .window = win,
                        .position = sp_iv2(e->event_x, e->event_y),
                    });
            } break;

            default:
                break;
        }
        free(ev);
    }

    // Handle composite text events like diacritics.
    while (XPending(linux_state.xdpy)) {
        XEvent ev;
        XNextEvent(linux_state.xdpy, &ev);
        if (ev.type == KeyPress) {
            XKeyEvent* e = (XKeyEvent*) &ev;
            if (XFilterEvent((XEvent*) e, e->window)) {
                continue;
            }

            cit_window* win = get_window_from_event(e->window);
            push_event(&first_event, &last_event, handle_text_event(win, e));
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
            // Keyboard
            XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_KEY_RELEASE |
            // Mouse
            XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION;
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
