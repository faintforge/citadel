#include "spire.h"
#ifdef SP_OS_LINUX

#include "internal.h"
#include <xcb/xcb.h>
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

cit_os_state os_state = {0};

b8 cit_os_init(void) {
    os_state.arena = sp_arena_create();
    sp_arena_tag(os_state.arena, sp_str_lit("os"));

    os_state.xdpy = XOpenDisplay(NULL);
    if (os_state.xdpy == NULL) {
        return false;
    }

    os_state.conn = XGetXCBConnection(os_state.xdpy);
    if (os_state.conn == NULL) {
        return false;
    }
    XSetEventQueueOwner(os_state.xdpy, XCBOwnsEventQueue);

    return true;
}

void cit_os_terminate(void) {
    XCloseDisplay(os_state.xdpy);
    sp_arena_destroy(os_state.arena);
}

#endif // SP_OS_LINUX
