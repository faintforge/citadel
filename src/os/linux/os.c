#include "spire.h"
#ifdef SP_OS_LINUX

#include "internal.h"
#include <xcb/xcb.h>

cit_os_state os_state = {0};

b8 cit_os_init(void) {
    os_state.arena = sp_arena_create();
    sp_arena_tag(os_state.arena, sp_str_lit("os"));

    os_state.conn = xcb_connect(NULL, NULL);
    if (os_state.conn == NULL) {
        return false;
    }

    return true;
}

void cit_os_terminate(void) {
    xcb_disconnect(os_state.conn);
    sp_arena_destroy(os_state.arena);
}

#endif // SP_OS_LINUX
