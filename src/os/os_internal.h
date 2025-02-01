#ifndef OS_INTERNAL_H_
#define OS_INTERNAL_H_

#include "spire.h"

typedef struct cit_os_gfx_interface cit_os_gfx_interface;
struct cit_os_gfx_interface {
    b8   (*init)(void);
    void (*terminate)(void);
};

extern cit_os_gfx_interface cit_os_gfx_gl_interface;
// extern cit_os_gfx_interface cit_os_gfx_vk_interface;

#endif // OS_INTERNAL_H
