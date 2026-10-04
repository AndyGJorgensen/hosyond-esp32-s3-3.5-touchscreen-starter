#pragma once

// Main screen hardware test controls: "Play SD sound" button, SD status line, mic level bar,
// backlight slider and RGB LED button.
void main_screen_init();  // after ui_init() and the managers' begin()
void main_screen_loop();  // from loop()
