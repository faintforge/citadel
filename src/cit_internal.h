#ifndef CIT_INTENRNAL_H_
#define CIT_INTENRNAL_H_

#include "citadel.h"

typedef b8 (*cit_backend_init_func)(cit_config config);
typedef void (*cit_backend_terminate_func)(void);
typedef cit_surface* (*cit_backend_surface_create)(cit_window* window);
typedef void (*cit_backend_surface_destroy)(cit_surface* surface);

typedef struct cit_backend_interface cit_backend_interface;
struct cit_backend_interface {
    cit_backend_init_func init;
    cit_backend_terminate_func terminate;
    cit_backend_surface_create surface_create;
    cit_backend_surface_destroy surface_destroy;
};

typedef struct cit_state cit_state;
struct cit_state {
    SP_Arena* arena;
    SP_Arena* events_arena;

    cit_backend_interface backend_interface;
};

extern cit_state _cit_state;

extern b8 cit_os_init(void);
extern void cit_os_terminate(void);

// Dummy
extern b8 cit_dummy_init(cit_config config);
extern void cit_dummy_terminate(void);
extern cit_surface* cit_dummy_surface_create(cit_window* window);
extern void cit_dummy_surface_destroy(cit_surface* surface);

static const cit_backend_interface CIT_BACKEND_DUMMY = {
    .init            = cit_dummy_init,
    .terminate       = cit_dummy_terminate,
    .surface_create  = cit_dummy_surface_create,
    .surface_destroy = cit_dummy_surface_destroy,
};

// OpenGL
extern b8 cit_os_gl_init(cit_config config);
extern void cit_os_gl_terminate(void);
extern cit_surface* cit_os_gl_surface_create(cit_window* window);
extern void cit_os_gl_surface_destroy(cit_surface* surface);

static const cit_backend_interface CIT_BACKEND_GL = {
    .init            = cit_os_gl_init,
    .terminate       = cit_os_gl_terminate,
    .surface_create  = cit_os_gl_surface_create,
    .surface_destroy = cit_os_gl_surface_destroy,
};

// Vulkan
extern b8 cit_vk_init(cit_config config);
extern void cit_vk_terminate(void);
extern cit_surface* cit_vk_surface_create(cit_window* window);
extern void cit_vk_surface_destroy(cit_surface* surface);

static const cit_backend_interface CIT_BACKEND_VK = {
    .init            = cit_vk_init,
    .terminate       = cit_vk_terminate,
    .surface_create  = cit_vk_surface_create,
    .surface_destroy = cit_vk_surface_destroy,
};

#endif // CIT_INTENRNAL_H_
