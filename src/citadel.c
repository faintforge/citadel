#define SPIRE_IMPLEMENTATION
#include "spire.h"

#include "citadel.h"
#include "internal.h"

b8 cit_init(cit_config config) {
    if (!cit_os_init()) { return false; }
    if (!cit_gfx_init(config)) { return false; }

    return true;
}

void cit_terminate(void) {
}
