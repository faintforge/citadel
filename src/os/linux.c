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

static cit_key translate_keysym(KeySym keysym) {
    switch (keysym) {
        // for (u32 uppercase = 'A'; uppercase <= 'Z'; uppercase++) {
        //     u32 lowercase = uppercase + 'a' - 'A';
        //     printf("case XK_%c:\ncase XK_%c:\n    return CIT_KEY_%c;\n", lowercase, uppercase, uppercase);
        // }
        case XK_a:
        case XK_A:
            return CIT_KEY_A;
        case XK_b:
        case XK_B:
            return CIT_KEY_B;
        case XK_c:
        case XK_C:
            return CIT_KEY_C;
        case XK_d:
        case XK_D:
            return CIT_KEY_D;
        case XK_e:
        case XK_E:
            return CIT_KEY_E;
        case XK_f:
        case XK_F:
            return CIT_KEY_F;
        case XK_g:
        case XK_G:
            return CIT_KEY_G;
        case XK_h:
        case XK_H:
            return CIT_KEY_H;
        case XK_i:
        case XK_I:
            return CIT_KEY_I;
        case XK_j:
        case XK_J:
            return CIT_KEY_J;
        case XK_k:
        case XK_K:
            return CIT_KEY_K;
        case XK_l:
        case XK_L:
            return CIT_KEY_L;
        case XK_m:
        case XK_M:
            return CIT_KEY_M;
        case XK_n:
        case XK_N:
            return CIT_KEY_N;
        case XK_o:
        case XK_O:
            return CIT_KEY_O;
        case XK_p:
        case XK_P:
            return CIT_KEY_P;
        case XK_q:
        case XK_Q:
            return CIT_KEY_Q;
        case XK_r:
        case XK_R:
            return CIT_KEY_R;
        case XK_s:
        case XK_S:
            return CIT_KEY_S;
        case XK_t:
        case XK_T:
            return CIT_KEY_T;
        case XK_u:
        case XK_U:
            return CIT_KEY_U;
        case XK_v:
        case XK_V:
            return CIT_KEY_V;
        case XK_w:
        case XK_W:
            return CIT_KEY_W;
        case XK_x:
        case XK_X:
            return CIT_KEY_X;
        case XK_y:
        case XK_Y:
            return CIT_KEY_Y;
        case XK_z:
        case XK_Z:
            return CIT_KEY_Z;

        // for (u32 i = 0; i <= 9; i++) {
        //     printf("case XK_%d: return CIT_KEY_%d;\n", i, i);
        // }
        case XK_0: return CIT_KEY_0;
        case XK_1: return CIT_KEY_1;
        case XK_2: return CIT_KEY_2;
        case XK_3: return CIT_KEY_3;
        case XK_4: return CIT_KEY_4;
        case XK_5: return CIT_KEY_5;
        case XK_6: return CIT_KEY_6;
        case XK_7: return CIT_KEY_7;
        case XK_8: return CIT_KEY_8;
        case XK_9: return CIT_KEY_9;

        // for (u32 i = 1; i <= 12; i++) {
        //     printf("case XK_F%d: return CIT_KEY_F%d;\n", i, i);
        // }
        case XK_F1: return CIT_KEY_F1;
        case XK_F2: return CIT_KEY_F2;
        case XK_F3: return CIT_KEY_F3;
        case XK_F4: return CIT_KEY_F4;
        case XK_F5: return CIT_KEY_F5;
        case XK_F6: return CIT_KEY_F6;
        case XK_F7: return CIT_KEY_F7;
        case XK_F8: return CIT_KEY_F8;
        case XK_F9: return CIT_KEY_F9;
        case XK_F10: return CIT_KEY_F10;
        case XK_F11: return CIT_KEY_F11;
        case XK_F12: return CIT_KEY_F12;

        case XK_Escape: return CIT_KEY_ESC;
        case XK_Tab: return CIT_KEY_TAB;
        case XK_BackSpace: return CIT_KEY_BACKSPACE;
        case XK_Return: return CIT_KEY_ENTER;

        case XK_Up: return CIT_KEY_ARROW_UP;
        case XK_Left: return CIT_KEY_ARROW_LEFT;
        case XK_Down: return CIT_KEY_ARROW_DOWN;
        case XK_Right: return CIT_KEY_ARROW_RIGHT;

        case XK_Caps_Lock: return CIT_KEY_CAPS_LOCK;
        case XK_Shift_L: return CIT_KEY_SHIFT_L;
        case XK_Shift_R: return CIT_KEY_SHIFT_R;
        case XK_Control_L: return CIT_KEY_CTRL_L;
        case XK_Control_R: return CIT_KEY_CTRL_R;
        case XK_Alt_L: return CIT_KEY_ALT_L;
        case XK_Alt_R: return CIT_KEY_ALT_R;
        case XK_Super_L: return CIT_KEY_SUPER_L;
        case XK_Super_R: return CIT_KEY_SUPER_R;

        default:
            return CIT_KEY_UNKNOWN;
    }
}

static cit_event handle_raw_key_event(cit_window* win, XKeyEvent* ev) {
    cit_event cit_ev = {
        .window = win,
    };

    KeySym keysym = XLookupKeysym(ev, 0);
    cit_ev.key = translate_keysym(keysym);

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
    cit_ev.key = translate_keysym(keysym);

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

Display* cit_native_linux_get_x11_display(void) {
    return linux_state.xdpy;
}

xcb_connection_t* cit_native_linux_get_xcb_connection(void) {
    return linux_state.conn;
}

xcb_window_t cit_native_linux_get_xcb_window_handle(const cit_window* window) {
    return ((linux_window*) window->internal)->handle;
}

#endif // SP_OS_LINUX
