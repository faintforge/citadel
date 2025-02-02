#include "../internal.h"
#include "citadel.h"
#include "spire.h"
#include "gfx_internal.h"

b8 cit_gfx_init(cit_config config) {
    switch (config.backend) {
        case CIT_GFX_BACKEND_OPENGL:
            cit_gfx_interface_curr = cit_gfx_opengl_interface;
            break;
        case CIT_GFX_BACKEND_VULKAN:
            cit_gfx_interface_curr = cit_gfx_vulkan_interface;
            break;
    }
    return cit_gfx_interface_curr.init(config);
}

void cit_gfx_terminate(void) {
    cit_gfx_interface_curr.terminate();
}
