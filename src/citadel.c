#include "spire.h"

#include "citadel.h"
#include "cit_internal.h"

cit_state _cit_state = {0};

static b8 cit_backend_interface_valid(cit_backend_interface interface) {
    return interface.init &&
        interface.terminate &&
        interface.surface_create &&
        interface.surface_destroy;
}

b8 cit_init(cit_config config) {
    switch (config.backend) {
        case CIT_GFX_BACKEND_NONE:
            _cit_state.backend_interface = CIT_BACKEND_DUMMY;
            break;
        case CIT_GFX_BACKEND_OPENGL:
            _cit_state.backend_interface = CIT_BACKEND_GL;
            break;
        case CIT_GFX_BACKEND_VULKAN:
            _cit_state.backend_interface = CIT_BACKEND_VK;
            sp_assert(false, "Vulkan backend currently not supported!");
            break;
    }
    sp_assert(cit_backend_interface_valid(_cit_state.backend_interface),
            "Incomplete backend provided!");

    _cit_state.arena = sp_arena_create();
    sp_arena_tag(_cit_state.arena, sp_str_lit("citadel"));

    _cit_state.events_arena = sp_arena_create();
    sp_arena_tag(_cit_state.events_arena, sp_str_lit("events"));

    if (!cit_os_init()) { return false; }
    if (!_cit_state.backend_interface.init(config)) { return false; }

    return true;
}

void cit_terminate(void) {
    _cit_state.backend_interface.terminate();
    cit_os_terminate();
    sp_arena_destroy(_cit_state.events_arena);
    sp_arena_destroy(_cit_state.arena);
}

cit_surface* cit_surface_create(cit_window* window) {
    return _cit_state.backend_interface.surface_create(window);
}

void cit_surface_destroy(cit_surface* surface) {
    _cit_state.backend_interface.surface_destroy(surface);
}
