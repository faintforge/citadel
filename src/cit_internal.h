#ifndef CIT_INTENRNAL_H_
#define CIT_INTENRNAL_H_

#include "citadel.h"

typedef struct cit_state cit_state;
struct cit_state {
    SP_Arena* arena;
    SP_Arena* events_arena;

    b8 (*gfx_init)(cit_config config);
    void (*gfx_terminate)(void);
};

extern cit_state _cit_state;

extern b8 cit_os_init(void);
extern void cit_os_terminate(void);

// Dummy
extern b8 cit_dummy_init(cit_config config);
extern void cit_dummy_terminate(void);

// OpenGL
extern b8 cit_os_gl_init(cit_config config);
extern void cit_os_gl_terminate(void);

// Vulkan
extern b8 cit_vk_init(cit_config config);
extern void cit_vk_terminate(void);

#endif // CIT_INTENRNAL_H_
