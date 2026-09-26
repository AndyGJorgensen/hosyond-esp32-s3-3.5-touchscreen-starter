#pragma once

// Registers the ST77922 panel as the LVGL display (rotation, 4-px alignment, flush).
// Call after lv_init(). Returns false if the panel or buffers could not be set up.
bool lvgl_display_init();
