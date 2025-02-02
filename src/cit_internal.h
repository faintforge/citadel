#ifndef CIT_INTENRNAL_H_
#define CIT_INTENRNAL_H_

#include "citadel.h"

typedef struct cit_state cit_state;
struct cit_state {
    SP_Arena* arena;

    b8 (*gfx_init)(cit_config config);
    void (*gfx_terminate)(void);
    cit_window* (*window_create)(cit_window_desc desc);
    void (*window_destroy)(cit_window* window);

    cit_window* window_stack;
};

struct cit_window {
    cit_window* next;
    b8 is_open;
    void* internal;
};

extern cit_state _cit_state;

extern b8 cit_os_init(void);
extern void cit_os_terminate(void);

// OpenGL
extern b8 cit_os_gl_init(cit_config config);
extern void cit_os_gl_terminate(void);
extern cit_window* cit_os_gl_window_create(cit_window_desc desc);
extern void cit_os_gl_window_destroy(cit_window* window);

// Vulkan
extern b8 cit_vk_init(cit_config config);
extern void cit_vk_terminate(void);
extern cit_window* cit_os_vk_window_create(cit_window_desc desc);
extern void cit_os_vk_window_destroy(cit_window* window);

#endif // CIT_INTENRNAL_H_
