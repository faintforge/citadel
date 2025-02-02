#include "spire.h"
#ifdef SP_OS_LINUX

#include "linux_internal.h"
#include "../cit_internal.h"
#include <xcb/xcb.h>
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

#endif // SP_OS_LINUX
