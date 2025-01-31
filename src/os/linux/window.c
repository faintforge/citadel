#include "spire.h"
#ifdef SP_OS_LINUX

#include "internal.h"
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_icccm.h>

cit_window* cit_window_create(SP_Ivec2 size, SP_Str title, b8 resizable) {
    cit_window* window = sp_arena_push_no_zero(os_state.arena, sizeof(cit_window));

    const xcb_setup_t* setup = xcb_get_setup(os_state.conn);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

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
            screen->root_visual,
            0, NULL);

    // Set window properties
    // https://xcb.freedesktop.org/windowcontextandmanipulation/
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
        xcb_icccm_set_wm_size_hints(os_state.conn, window->handle, XCB_ATOM_WM_NORMAL_HINTS, &hints);
    }

    // Show the window
    xcb_map_window(os_state.conn, window->handle);
    xcb_flush(os_state.conn);

    return window;
}

void cit_window_destroy(cit_window* window) {
    xcb_destroy_window(os_state.conn, window->handle);
}

#endif // SP_OS_LINUX
