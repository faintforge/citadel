#ifndef LINUX_INTERNAL_H_
#define LINUX_INTERNAL_H_

#include <X11/Xlib.h>
#include <xcb/xcb.h>

typedef struct cit_linux_state cit_linux_state;
struct cit_linux_state {
    Display* xdpy;
    xcb_connection_t* conn;
};
// Defined in linux.c
extern cit_linux_state linux_state;

typedef struct linux_window linux_window;
struct linux_window {
    xcb_window_t handle;
    xcb_atom_t destroy_atom;
    void* internal;
};

#endif // LINUX_INTERNAL_H_
