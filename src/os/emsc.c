#include "spire.h"
#ifdef SP_OS_EMSCRIPTEN

#include "../cit_internal.h"
#include <emscripten.h>
#include <emscripten/html5.h>

EM_JS(void, test_js, (void), {
    console.log("Test");
})

struct cit_window {
    SP_Ivec2 size;
};

typedef struct emsc_state emsc_state;
struct emsc_state {
    SP_Arena* events_arena;
    cit_event* event_stack;
};
static emsc_state _emsc_state = {0};

static void push_event(cit_event event) {
    cit_event* ev = sp_arena_push_no_zero(_emsc_state.events_arena, sizeof(cit_event));
    *ev = event;
    ev->next = _emsc_state.event_stack;
    _emsc_state.event_stack = ev;
}

b8 cit_os_init(void) {
    _emsc_state = (emsc_state) {
        .events_arena = sp_arena_create(),
    };
    return true;
}

void cit_os_terminate(void) {
    sp_arena_destroy(_emsc_state.events_arena);
}

cit_event* cit_poll_events(void) {
    // Hand back control to the browser as to not hang the application.
    emscripten_sleep(0);

    // Use an intermediate event stack because emscripten sends them as they
    // happen in a callback.
    sp_arena_clear(_cit_state.events_arena);
    cit_event* events = NULL;
    for (cit_event* ev = _emsc_state.event_stack; ev != NULL; ev = ev->next) {
        cit_event* event = sp_arena_push_no_zero(_cit_state.events_arena, sizeof(cit_event));
        *event = *ev;
        event->next = events;
        events = event;
    }
    sp_arena_clear(_emsc_state.events_arena);
    _emsc_state.event_stack = NULL;

    return events;
}

static bool resize_callback(int eventType, const EmscriptenUiEvent *uiEvent __attribute__((nonnull)), void *userData) {
    (void) eventType;
    cit_window* window = userData;
    window->size.x = uiEvent->windowInnerWidth;
    window->size.y = uiEvent->windowInnerHeight;

    push_event((cit_event) {
        .type = CIT_EVENT_TYPE_WINDOW_RESIZE,
        .window = window,
        .size = window->size,
    });

    return true;
}

static cit_key translate_keycode(SP_Str key) {
    // for (u32 i = 0; i <= 9; i++) {
    //     printf("else if (sp_str_equal(key, sp_str_lit(\"Digit%d\"))) { return CIT_KEY_%d; }\n", i, i);
    // }
    if (sp_str_equal(key, sp_str_lit("Digit0"))) { return CIT_KEY_0; }
    else if (sp_str_equal(key, sp_str_lit("Digit1"))) { return CIT_KEY_1; }
    else if (sp_str_equal(key, sp_str_lit("Digit2"))) { return CIT_KEY_2; }
    else if (sp_str_equal(key, sp_str_lit("Digit3"))) { return CIT_KEY_3; }
    else if (sp_str_equal(key, sp_str_lit("Digit4"))) { return CIT_KEY_4; }
    else if (sp_str_equal(key, sp_str_lit("Digit5"))) { return CIT_KEY_5; }
    else if (sp_str_equal(key, sp_str_lit("Digit6"))) { return CIT_KEY_6; }
    else if (sp_str_equal(key, sp_str_lit("Digit7"))) { return CIT_KEY_7; }
    else if (sp_str_equal(key, sp_str_lit("Digit8"))) { return CIT_KEY_8; }
    else if (sp_str_equal(key, sp_str_lit("Digit9"))) { return CIT_KEY_9; }

    // for (u32 uppercase = 'A'; uppercase <= 'Z'; uppercase++) {
    //     printf("else if (sp_str_equal(key, sp_str_lit(\"Key%c\"))) { return CIT_KEY_%c; }\n", uppercase, uppercase);
    // }
    else if (sp_str_equal(key, sp_str_lit("KeyA"))) { return CIT_KEY_A; }
    else if (sp_str_equal(key, sp_str_lit("KeyB"))) { return CIT_KEY_B; }
    else if (sp_str_equal(key, sp_str_lit("KeyC"))) { return CIT_KEY_C; }
    else if (sp_str_equal(key, sp_str_lit("KeyD"))) { return CIT_KEY_D; }
    else if (sp_str_equal(key, sp_str_lit("KeyE"))) { return CIT_KEY_E; }
    else if (sp_str_equal(key, sp_str_lit("KeyF"))) { return CIT_KEY_F; }
    else if (sp_str_equal(key, sp_str_lit("KeyG"))) { return CIT_KEY_G; }
    else if (sp_str_equal(key, sp_str_lit("KeyH"))) { return CIT_KEY_H; }
    else if (sp_str_equal(key, sp_str_lit("KeyI"))) { return CIT_KEY_I; }
    else if (sp_str_equal(key, sp_str_lit("KeyJ"))) { return CIT_KEY_J; }
    else if (sp_str_equal(key, sp_str_lit("KeyK"))) { return CIT_KEY_K; }
    else if (sp_str_equal(key, sp_str_lit("KeyL"))) { return CIT_KEY_L; }
    else if (sp_str_equal(key, sp_str_lit("KeyM"))) { return CIT_KEY_M; }
    else if (sp_str_equal(key, sp_str_lit("KeyN"))) { return CIT_KEY_N; }
    else if (sp_str_equal(key, sp_str_lit("KeyO"))) { return CIT_KEY_O; }
    else if (sp_str_equal(key, sp_str_lit("KeyP"))) { return CIT_KEY_P; }
    else if (sp_str_equal(key, sp_str_lit("KeyQ"))) { return CIT_KEY_Q; }
    else if (sp_str_equal(key, sp_str_lit("KeyR"))) { return CIT_KEY_R; }
    else if (sp_str_equal(key, sp_str_lit("KeyS"))) { return CIT_KEY_S; }
    else if (sp_str_equal(key, sp_str_lit("KeyT"))) { return CIT_KEY_T; }
    else if (sp_str_equal(key, sp_str_lit("KeyU"))) { return CIT_KEY_U; }
    else if (sp_str_equal(key, sp_str_lit("KeyV"))) { return CIT_KEY_V; }
    else if (sp_str_equal(key, sp_str_lit("KeyW"))) { return CIT_KEY_W; }
    else if (sp_str_equal(key, sp_str_lit("KeyX"))) { return CIT_KEY_X; }
    else if (sp_str_equal(key, sp_str_lit("KeyY"))) { return CIT_KEY_Y; }
    else if (sp_str_equal(key, sp_str_lit("KeyZ"))) { return CIT_KEY_Z; }

    // for (u32 i = 1; i <= 12; i++) {
    //     printf("else if (sp_str_equal(key, sp_str_lit(\"F%d\"))) { return CIT_KEY_F%d; }\n", i, i);
    // }
    else if (sp_str_equal(key, sp_str_lit("F1"))) { return CIT_KEY_F1; }
    else if (sp_str_equal(key, sp_str_lit("F2"))) { return CIT_KEY_F2; }
    else if (sp_str_equal(key, sp_str_lit("F3"))) { return CIT_KEY_F3; }
    else if (sp_str_equal(key, sp_str_lit("F4"))) { return CIT_KEY_F4; }
    else if (sp_str_equal(key, sp_str_lit("F5"))) { return CIT_KEY_F5; }
    else if (sp_str_equal(key, sp_str_lit("F6"))) { return CIT_KEY_F6; }
    else if (sp_str_equal(key, sp_str_lit("F7"))) { return CIT_KEY_F7; }
    else if (sp_str_equal(key, sp_str_lit("F8"))) { return CIT_KEY_F8; }
    else if (sp_str_equal(key, sp_str_lit("F9"))) { return CIT_KEY_F9; }
    else if (sp_str_equal(key, sp_str_lit("F10"))) { return CIT_KEY_F10; }
    else if (sp_str_equal(key, sp_str_lit("F11"))) { return CIT_KEY_F11; }
    else if (sp_str_equal(key, sp_str_lit("F12"))) { return CIT_KEY_F12; }

    else if (sp_str_equal(key, sp_str_lit("Escape"))) { return CIT_KEY_ESC; }
    else if (sp_str_equal(key, sp_str_lit("Tab"))) { return CIT_KEY_TAB; }
    else if (sp_str_equal(key, sp_str_lit("Backspace"))) { return CIT_KEY_BACKSPACE; }
    else if (sp_str_equal(key, sp_str_lit("Enter"))) { return CIT_KEY_ENTER; }

    else if (sp_str_equal(key, sp_str_lit("ArrowUp"))) { return CIT_KEY_ARROW_UP; }
    else if (sp_str_equal(key, sp_str_lit("ArrowLeft"))) { return CIT_KEY_ARROW_LEFT; }
    else if (sp_str_equal(key, sp_str_lit("ArrowDown"))) { return CIT_KEY_ARROW_DOWN; }
    else if (sp_str_equal(key, sp_str_lit("ArrowRight"))) { return CIT_KEY_ARROW_RIGHT; }

    else if (sp_str_equal(key, sp_str_lit("CapsLock"))) { return CIT_KEY_CAPS_LOCK; }
    else if (sp_str_equal(key, sp_str_lit("ShiftLeft"))) { return CIT_KEY_SHIFT_L; }
    else if (sp_str_equal(key, sp_str_lit("ShiftRight"))) { return CIT_KEY_SHIFT_R; }
    else if (sp_str_equal(key, sp_str_lit("ControlLeft"))) { return CIT_KEY_CTRL_L; }
    else if (sp_str_equal(key, sp_str_lit("ControlRight"))) { return CIT_KEY_CTRL_R; }
    else if (sp_str_equal(key, sp_str_lit("AltLeft"))) { return CIT_KEY_ALT_L; }
    else if (sp_str_equal(key, sp_str_lit("AltRight"))) { return CIT_KEY_ALT_R; }
    else if (sp_str_equal(key, sp_str_lit("MetaLeft"))) { return CIT_KEY_SUPER_L; }
    else if (sp_str_equal(key, sp_str_lit("MetaRight"))) { return CIT_KEY_SUPER_R; }

    return CIT_KEY_UNKNOWN;
}

static bool key_callback(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    if (keyEvent->repeat) {
        return true;
    }

    cit_window* window = userData;
    cit_event_type type = CIT_EVENT_TYPE_NONE;
    switch (eventType) {
        case EMSCRIPTEN_EVENT_KEYDOWN:
            type = CIT_EVENT_TYPE_KEY_PRESS;
            break;
        case EMSCRIPTEN_EVENT_KEYUP:
            type = CIT_EVENT_TYPE_KEY_RELEASE;
            break;
        default:
            break;
    }

    cit_mod mod = CIT_MOD_NONE;
    if (keyEvent->ctrlKey)  { mod |= CIT_MOD_CRTL; }
    if (keyEvent->altKey)   { mod |= CIT_MOD_ALT_L | CIT_MOD_ALT_R; }
    if (keyEvent->shiftKey) { mod |= CIT_MOD_SHIFT; }

    push_event((cit_event) {
            .type = type,
            .window = window,
            .size = window->size,
            .key = translate_keycode(sp_cstr(keyEvent->code)),
            .mod = mod,
        });

    return true;
}

static bool text_callback(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
    (void) eventType;
    cit_window* window = userData;
    // Only support ASCII characters.
    if (keyEvent->key[1] != 0) {
        if (sp_str_equal(sp_cstr(keyEvent->key), sp_str_lit("Enter"))) {
            push_event((cit_event) {
                    .type = CIT_EVENT_TYPE_TEXT,
                    .window = window,
                    .key = translate_keycode(sp_cstr(keyEvent->code)),
                    .codepoint = 13,
                });
        } else if (sp_str_equal(sp_cstr(keyEvent->key), sp_str_lit("Backspace"))) {
            push_event((cit_event) {
                    .type = CIT_EVENT_TYPE_TEXT,
                    .window = window,
                    .key = translate_keycode(sp_cstr(keyEvent->code)),
                    .codepoint = 8,
                });
        }

        return true;
    }

    cit_mod mod = CIT_MOD_NONE;
    if (keyEvent->ctrlKey)  { mod |= CIT_MOD_CRTL; }
    if (keyEvent->altKey)   { mod |= CIT_MOD_ALT_L | CIT_MOD_ALT_R; }
    if (keyEvent->shiftKey) { mod |= CIT_MOD_SHIFT; }

    push_event((cit_event) {
            .type = CIT_EVENT_TYPE_TEXT,
            .window = window,
            .key = translate_keycode(sp_cstr(keyEvent->code)),
            .codepoint = keyEvent->key[0],
            .mod = mod,
        });

    return true;
}

static bool mouse_move_callback(int eventType, const EmscriptenMouseEvent *mouseEvent __attribute__((nonnull)), void *userData) {
    (void) eventType;
    cit_window* window = userData;
    cit_mod mod = CIT_MOD_NONE;
    if (mouseEvent->ctrlKey)  { mod |= CIT_MOD_CRTL; }
    if (mouseEvent->altKey)   { mod |= CIT_MOD_ALT_L | CIT_MOD_ALT_R; }
    if (mouseEvent->shiftKey) { mod |= CIT_MOD_SHIFT; }
    push_event((cit_event) {
            .type = CIT_EVENT_TYPE_MOUSE_MOVE,
            .window = window,
            .position = sp_iv2(mouseEvent->targetX, mouseEvent->targetY),
            .mod = mod,
        });
    return true;
}

static bool mouse_button_callback(int eventType, const EmscriptenMouseEvent *mouseEvent __attribute__((nonnull)), void *userData) {
    cit_window* window = userData;
    cit_event_type type = CIT_EVENT_TYPE_NONE;
    switch (eventType) {
        case EMSCRIPTEN_EVENT_MOUSEDOWN:
            type = CIT_EVENT_TYPE_MOUSE_BUTTON_PRESS;
            break;
        case EMSCRIPTEN_EVENT_MOUSEUP:
            type = CIT_EVENT_TYPE_MOUSE_BUTTON_RELEASE;
            break;
        default:
            sp_debug("here");
            break;
    }

    cit_mouse_button button = CIT_MOUSE_BUTTON_LEFT;
    switch (mouseEvent->button) {
        case 0:
            button = CIT_MOUSE_BUTTON_LEFT;
            break;
        case 1:
            button = CIT_MOUSE_BUTTON_MIDDLE;
            break;
        case 2:
            button = CIT_MOUSE_BUTTON_RIGHT;
            break;
    }

    cit_mod mod = CIT_MOD_NONE;
    if (mouseEvent->ctrlKey)  { mod |= CIT_MOD_CRTL; }
    if (mouseEvent->altKey)   { mod |= CIT_MOD_ALT_L | CIT_MOD_ALT_R; }
    if (mouseEvent->shiftKey) { mod |= CIT_MOD_SHIFT; }

    push_event((cit_event) {
            .type = type,
            .window = window,
            .mod = mod,
            .position = sp_iv2(mouseEvent->targetX, mouseEvent->targetY),
            .button = button,
        });

    return false;
}

// static bool scroll_callback(int eventType, const EmscriptenUiEvent *uiEvent __attribute__((nonnull)), void *userData) {
//     (void) eventType;
//     sp_debug("Here");
//     cit_window* window = userData;
//     push_event((cit_event) {
//             .type = CIT_EVENT_TYPE_MOUSE_SCROLL,
//             .window = window,
//             .scroll = uiEvent->scrollTop,
//         });
//     return true;
// }

static bool mouse_scroll_callback(int eventType, const EmscriptenWheelEvent *wheelEvent __attribute__((nonnull)), void *userData) {
    (void) eventType;
    cit_window* window = userData;
    const EmscriptenMouseEvent* mouseEvent = &wheelEvent->mouse;

    cit_mod mod = CIT_MOD_NONE;
    if (mouseEvent->ctrlKey)  { mod |= CIT_MOD_CRTL; }
    if (mouseEvent->altKey)   { mod |= CIT_MOD_ALT_L | CIT_MOD_ALT_R; }
    if (mouseEvent->shiftKey) { mod |= CIT_MOD_SHIFT; }

    push_event((cit_event) {
            .type = CIT_EVENT_TYPE_MOUSE_SCROLL,
            .window = window,
            .scroll = wheelEvent->deltaY > 0 ? -1 : 1,
            .mod = mod,
            .position = sp_iv2(mouseEvent->targetX, mouseEvent->targetY),
        });

    return true;
}

cit_window* cit_window_create(cit_window_desc desc) {
    static b8 window_has_been_created = false;
    sp_assert(!window_has_been_created, "Emscripten only supports a single window!");
    window_has_been_created = true;

    cit_window* window = sp_arena_push_no_zero(_cit_state.arena, sizeof(cit_window));
    window->size = desc.size;

    SP_Scratch scratch = sp_scratch_begin(NULL, 0);
    const char* cstr_title = sp_str_to_cstr(scratch.arena, desc.title);
    emscripten_set_window_title(cstr_title);

    SP_Str id_str = sp_str_pushf(scratch.arena, "#%.*s", desc.title.len, desc.title.data);
    const char* id = sp_str_to_cstr(scratch.arena, id_str);

    emscripten_set_resize_callback(id, window, true, resize_callback);

    emscripten_set_keyup_callback(id, window, true, key_callback);
    emscripten_set_keydown_callback(id, window, true, key_callback);
    emscripten_set_keydown_callback(id, window, true, text_callback);

    emscripten_set_mousemove_callback(id, window, true, mouse_move_callback);
    emscripten_set_mouseup_callback(id, window, true, mouse_button_callback);
    emscripten_set_mousedown_callback(id, window, true, mouse_button_callback);
    emscripten_set_wheel_callback(id, window, true, mouse_scroll_callback);

    emscripten_set_canvas_element_size(id, desc.size.x, desc.size.y);

    sp_scratch_end(scratch);

    return window;
}

void cit_window_destroy(cit_window* window) {
    (void) window;
}

SP_Ivec2 cit_window_get_size(const cit_window* window) {
    return window->size;
}

#endif // SP_OS_EMSCRIPTEN
