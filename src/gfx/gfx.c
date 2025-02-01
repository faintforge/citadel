#include "citadel.h"
#include "../os/os_internal.h"


b8 cit_gfx_init(void) {
    return cit_os_gfx_gl_interface.init();
}

void cit_gfx_terminate(void) {
    cit_os_gfx_gl_interface.terminate();
}
