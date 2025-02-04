#include "spire.h"

#include "citadel.h"
#include "cit_internal.h"

cit_state _cit_state = {0};

b8 cit_init(cit_config config) {
    switch (config.backend) {
        case CIT_GFX_BACKEND_OPENGL:
            _cit_state = (cit_state) {
                .gfx_init = cit_os_gl_init,
                .gfx_terminate = cit_os_gl_terminate,
                .window_create = cit_os_gl_window_create,
                .window_destroy = cit_os_gl_window_destroy,
            };
            break;
        case CIT_GFX_BACKEND_VULKAN:
            _cit_state = (cit_state) {
                .gfx_init = cit_vk_init,
                .gfx_terminate = cit_vk_terminate,
                .window_create = cit_os_vk_window_create,
                .window_destroy = cit_os_vk_window_destroy,
            };
            sp_assert(false, "Vulkan backend currently not supported!");
            break;
    }

    _cit_state.arena = sp_arena_create();
    sp_arena_tag(_cit_state.arena, sp_str_lit("citadel"));

    _cit_state.events_arena = sp_arena_create();
    sp_arena_tag(_cit_state.events_arena, sp_str_lit("events"));

    if (!cit_os_init()) { return false; }
    if (!_cit_state.gfx_init(config)) { return false; }

    return true;
}

void cit_terminate(void) {
    _cit_state.gfx_terminate();
    cit_os_terminate();
    sp_arena_destroy(_cit_state.events_arena);
    sp_arena_destroy(_cit_state.arena);
}

cit_window* cit_window_create(cit_window_desc desc) {
    cit_window* window = _cit_state.window_create(desc);
    window->next = _cit_state.window_stack;
    _cit_state.window_stack = window;
    return window;
}

void cit_window_destroy(cit_window* window) {
    _cit_state.window_destroy(window);
}
