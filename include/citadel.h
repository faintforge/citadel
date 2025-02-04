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

typedef struct cit_window_desc cit_window_desc;
struct cit_window_desc {
    SP_Ivec2 size;
    SP_Str title;
    b8 resizable;
};

typedef struct cit_window cit_window;

extern cit_window* cit_window_create(cit_window_desc desc);
extern void        cit_window_destroy(cit_window* window);

// Events

typedef enum cit_event_type {
    CIT_EVENT_TYPE_NONE,
    CIT_EVENT_TYPE_WINDOW_CLOSE,
    CIT_EVENT_TYPE_WINDOW_RESIZE,
    CIT_EVENT_TYPE_KEY_PRESS,
    CIT_EVENT_TYPE_KEY_RELEASE,
    CIT_EVENT_TYPE_TEXT,
    CIT_EVENT_TYPE_MOUSE_BUTTON_PRESS,
    CIT_EVENT_TYPE_MOUSE_BUTTON_RELEASE,
    CIT_EVENT_TYPE_MOUSE_MOVE,
} cit_event_type;

typedef enum cit_mod {
    CIT_MOD_NONE  = 0,
    CIT_MOD_SHIFT = 1 << 0,
    CIT_MOD_CRTL  = 1 << 1,
    CIT_MOD_ALT_L = 1 << 2,
    CIT_MOD_ALT_R = 1 << 3,
} cit_mod;

typedef enum cit_mouse_button : i8 {
    CIT_MOUSE_BUTTON_LEFT,
    CIT_MOUSE_BUTTON_RIGHT,
    CIT_MOUSE_BUTTON_MIDDLE,
} cit_mouse_button;

typedef struct cit_event cit_event;
struct cit_event {
    cit_event* next;
    cit_event* prev;

    cit_event_type type;
    cit_window* window;

    cit_mod mod;
    // cit_key key;
    u32 scancode;
    u32 codepoint;
    cit_mouse_button button;
    SP_Ivec2 position;
    SP_Ivec2 delta;
    SP_Ivec2 size;
};

extern cit_event* cit_poll_events(void);

#endif // CITADEL_H_
