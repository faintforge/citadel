#include "spire.h"
#ifdef SP_OS_LINUX

#include "citadel.h"

#include <xcb/xcb.h>

typedef struct cit_os_state cit_os_state;
struct cit_os_state {
    SP_Arena* arena;
    xcb_connection_t* conn;
};
// Defined in os.c
extern cit_os_state os_state;

struct cit_window {
    xcb_window_t handle;
};

#endif // SP_OS_LINUX
