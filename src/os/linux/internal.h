#include "spire.h"
#ifdef SP_OS_LINUX

#include "citadel.h"

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <X11/Xlib.h>

struct cit_window {
    cit_window* next;
    xcb_window_t handle;
    xcb_atom_t destroy_atom;
    b8 is_open;
    void* gfx_data;
};

typedef struct cit_os_state cit_os_state;
struct cit_os_state {
    SP_Arena* arena;
    Display* xdpy;
    xcb_connection_t* conn;
    cit_window* window_stack;
};
// Defined in os.c
extern cit_os_state os_state;

#endif // SP_OS_LINUX
