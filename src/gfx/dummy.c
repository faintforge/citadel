#include "../cit_internal.h"

b8 cit_dummy_init(cit_config config) {
    (void) config;
    return true;
}

void cit_dummy_terminate(void) {}

cit_surface* cit_dummy_surface_create(cit_window* window) {
    (void) window;
    return NULL;
}

void cit_dummy_surface_destroy(cit_surface* surface) {
    (void) surface;
}
