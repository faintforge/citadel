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

cit_window* cit_os_vk_window_create(cit_window_desc desc) {
    (void) desc;
    sp_assert(false, "%s not implemented!", __func__);
    return NULL;
}

void cit_os_vk_window_destroy(cit_window* window) {
    (void) window;
    sp_assert(false, "%s not implemented!", __func__);
}
