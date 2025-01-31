#ifndef CITADEL_H_
#define CITADEL_H_

#include "spire.h"

// OS Layer

extern b8   cit_os_init(void);
extern void cit_os_terminate(void);

// Windowing

typedef struct cit_window cit_window;

extern cit_window* cit_window_create(SP_Ivec2 size, SP_Str title, b8 resizable);
extern void        cit_window_destroy(cit_window* window);

#endif // CITADEL_H_
