#ifndef LINUX_INTERNAL_H_
#define LINUX_INTERNAL_H_

#include "spire.h"
#ifdef SP_OS_LINUX

#include "citadel.h"
#include <X11/Xlib.h>
#include <xcb/xcb.h>

typedef struct cit_linux_state cit_linux_state;
struct cit_linux_state {
    Display* xdpy;
    xcb_connection_t* conn;
    XIM xim;
    cit_window* window_stack;
    xcb_screen_t* screen;
};
// Defined in linux.c
extern cit_linux_state linux_state;

struct cit_window {
    cit_window* next;
    SP_Ivec2 size;
    xcb_window_t handle;
    xcb_atom_t destroy_atom;
    void* internal;
    XIC xic;
};

#endif // SP_OS_LINUX
#endif // LINUX_INTERNAL_H_
