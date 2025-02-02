#ifndef CITADEL_H_
#define CITADEL_H_

#include "spire.h"

typedef enum cit_gfx_backend {
    CIT_GFX_BACKEND_OPENGL,
    CIT_GFX_BACKEND_VULKAN,
} cit_gfx_backend;

typedef struct cit_config cit_config;
struct cit_config {
    cit_gfx_backend backend;
    struct {
        u32 version_major;
        u32 version_minor;
        b8 es;
    } gl;
    struct {} vk;
};

extern b8   cit_init(cit_config config);
extern void cit_terminate(void);

// Windowing

typedef struct cit_window cit_window;

extern cit_window* cit_window_create(SP_Ivec2 size, SP_Str title, b8 resizable);
extern void        cit_window_destroy(cit_window* window);
extern b8          cit_window_is_open(const cit_window* window);
extern void        cit_poll_events(void);

#endif // CITADEL_H_
