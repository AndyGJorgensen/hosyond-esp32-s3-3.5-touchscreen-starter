# Project guardrails

- Never modify anything under src/ui/.
- Treat src/ui/ as generated output from the design tool. Keep it read-only.
- Prefer fixes in project config, board settings, driver setup, global LVGL config, and library configuration.
- If a generator tries to overwrite src/ui/, reject the overwrite and regenerate from the design tool in a safe configuration instead.
- Do not edit generated screens, styles, vars, flow files, assets, or images in src/ui/ unless the user explicitly asks for it.  You can edit them to debug and find issues however they will get regenerator over and changes made to fix issues have to be done outside of files in the src/ui folder
- For any build or upload issue, trace the failure to config or driver code outside src/ui before making changes.
- upload changes do not build save time and just attempt to upload
- examples folder is not part of the project it includes examples that the seller included with the baord and has schematics and other important details about the baord  ignore looking in this folder inless asked to do so.  you may ask me to look in there if you feel the need to see schematics or expamples
- User-adjustable settings workflow: anything the user may want to change later (host name, feature defaults such as OTA on/off at boot, passwords, timeouts, screen orientation, etc.) goes in `include/app_config.h` as a named `#define` with a comment. Code reads it from there; never hard-code such values in other files. When adding a new feature, put its tunable values in that file. If a value must also appear in `platformio.ini` (e.g. OTA upload host/password), note the duplication in both places.
- EEZ Studio project work (screens, widgets, styles, fonts, flow, the .eez-project JSON): use the project skill in `.claude/skills/eezstudio/SKILL.md`. Its top "READ FIRST" section adapts it to this project (PlatformIO, src/ui, LVGL 9.5, flow enabled) and overrides the downloaded text where they conflict.
- update board.md with knowledge spcecific to the all in one board with esp32 lcd touchscreen speaker built in