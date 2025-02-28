#include "../cit_internal.h"
#include "spire.h"

b8 cit_vk_init(cit_config config) {
    (void) config;
    sp_assert(false, "%s not implemented!", __func__);
    return false;
}

void cit_vk_terminate(void) {
    sp_assert(false, "%s not implemented!", __func__);
}

cit_surface* cit_vk_surface_create(cit_window* window) {
    (void) window;
    sp_assert(false, "%s not implemented!", __func__);
    return NULL;
}

void cit_vk_surface_destroy(cit_surface* surface) {
    (void) surface;
    sp_assert(false, "%s not implemented!", __func__);
}
