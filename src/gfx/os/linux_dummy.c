#include "cit_internal.h"
#include "os/linux_internal.h"

// cit_window* cit_os_dummy_window_create(cit_window_desc desc) {
//     cit_window* win = sp_arena_push_no_zero(_cit_state.arena, sizeof(cit_window));
//     linux_window* lwin = internal_linux_window_create(desc, INTERNAL_LINUX_VISUAL_ID_DONT_CARE);
//     *win = (cit_window) {
//         .internal = lwin,
//     };
//     return win;
// }
//
// void cit_os_dummy_window_destroy(cit_window* window) {
//     internal_linux_window_destroy(window->internal);
// }
