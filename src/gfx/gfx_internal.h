#ifndef GFX_INTERNAL_H_
#define GFX_INTERNAL_H_

#include "citadel.h"

typedef struct cit_gfx_interface cit_gfx_interface;
struct cit_gfx_interface {
    b8 (*init)(cit_config config);
    void (*terminate)(void);
    cit_window* (*window_create)(cit_window_desc desc);
};

// Defined in gfx.c
extern cit_gfx_interface cit_gfx_interface_curr;

extern cit_gfx_interface cit_gfx_opengl_interface;
extern cit_gfx_interface cit_gfx_vulkan_interface;

#endif // GFX_INTERNAL_H_
