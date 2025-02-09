#ifndef CITADEL_H_
#define CITADEL_H_

#include "spire.h"

typedef enum cit_gfx_backend {
    CIT_GFX_BACKEND_NONE,
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
extern SP_Ivec2    cit_window_get_size(const cit_window* window);

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
    CIT_EVENT_TYPE_MOUSE_SCROLL,
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

typedef enum cit_key {
    CIT_KEY_UNKNOWN,

    CIT_KEY_0,
    CIT_KEY_1,
    CIT_KEY_2,
    CIT_KEY_3,
    CIT_KEY_4,
    CIT_KEY_5,
    CIT_KEY_6,
    CIT_KEY_7,
    CIT_KEY_8,
    CIT_KEY_9,

    CIT_KEY_F1,
    CIT_KEY_F2,
    CIT_KEY_F3,
    CIT_KEY_F4,
    CIT_KEY_F5,
    CIT_KEY_F6,
    CIT_KEY_F7,
    CIT_KEY_F8,
    CIT_KEY_F9,
    CIT_KEY_F10,
    CIT_KEY_F11,
    CIT_KEY_F12,

    CIT_KEY_A, // = 97,
    CIT_KEY_B, // = 98,
    CIT_KEY_C, // = 99,
    CIT_KEY_D, // = 100,
    CIT_KEY_E, // = 101,
    CIT_KEY_F, // = 102,
    CIT_KEY_G, // = 103,
    CIT_KEY_H, // = 104,
    CIT_KEY_I, // = 105,
    CIT_KEY_J, // = 106,
    CIT_KEY_K, // = 107,
    CIT_KEY_L, // = 108,
    CIT_KEY_M, // = 109,
    CIT_KEY_N, // = 110,
    CIT_KEY_O, // = 111,
    CIT_KEY_P, // = 112,
    CIT_KEY_Q, // = 113,
    CIT_KEY_R, // = 114,
    CIT_KEY_S, // = 115,
    CIT_KEY_T, // = 116,
    CIT_KEY_U, // = 117,
    CIT_KEY_V, // = 118,
    CIT_KEY_W, // = 119,
    CIT_KEY_X, // = 120,
    CIT_KEY_Y, // = 121,
    CIT_KEY_Z, // = 122,

    CIT_KEY_ESC,
    CIT_KEY_TAB,
    CIT_KEY_BACKSPACE,
    CIT_KEY_ENTER,

    CIT_KEY_ARROW_UP,
    CIT_KEY_ARROW_LEFT,
    CIT_KEY_ARROW_DOWN,
    CIT_KEY_ARROW_RIGHT,

    CIT_KEY_CAPS_LOCK,
    CIT_KEY_SHIFT_L,
    CIT_KEY_SHIFT_R,
    CIT_KEY_CTRL_L,
    CIT_KEY_CTRL_R,
    CIT_KEY_ALT_L,
    CIT_KEY_ALT_R,
    CIT_KEY_SUPER_L,
    CIT_KEY_SUPER_R,

    CIT_KEY_COUNT,
} cit_key;

typedef struct cit_event cit_event;
struct cit_event {
    cit_event* next;
    cit_event* prev;

    cit_event_type type;
    cit_window* window;

    cit_mod mod;
    cit_key key;
    u32 scancode;
    u32 codepoint;
    cit_mouse_button button;
    SP_Ivec2 position;
    SP_Ivec2 size;
    i8 scroll;
};

extern cit_event* cit_poll_events(void);

#endif // CITADEL_H_
