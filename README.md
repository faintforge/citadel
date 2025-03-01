# Citadel

## Emscripten
The Emscripten backend only supports OpenGL ES 2.0.

Text events are only triggered by ASCII characters including enter and backspace.

When creating a window with `cit_window_create` make sure the title matches the
ID of the canvas you want to target.
