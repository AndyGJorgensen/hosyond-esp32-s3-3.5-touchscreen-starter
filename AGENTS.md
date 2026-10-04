# Project guardrails

- Never modify anything under `src/ui/`.
- Treat `src/ui/` as generated output from the design tool (EEZ Studio). Keep it read-only.
- Prefer fixes in project config, board settings, driver setup, global LVGL config, and library configuration.
- If a generator tries to overwrite `src/ui/`, reject the overwrite and regenerate from the design tool in a safe configuration instead.
- Do not edit generated screens, styles, vars, flow files, assets, or images in `src/ui/` unless the user explicitly asks for it. You may read or temporarily edit them to debug, but they are regenerated on every export, so the actual fix must go in files outside `src/ui/`.
- For any build or upload issue, trace the failure to config or driver code outside `src/ui/` before making changes.
- To upload changes, don't run a separate build first: just run the upload, which builds as part of it.
- The board's vendor documents (schematic, datasheets, seller example code) are not part of this repo. `board/board.md` links to them, and local copies go in the git-ignored `board/datasheets/`. Don't go looking in those unless asked; ask the user if you need the schematic or vendor examples.
- User-adjustable settings workflow: anything the user may want to change later (host name, feature defaults such as OTA on/off at boot, passwords, timeouts, screen orientation, etc.) goes in `include/app_config.h` as a named `#define` with a comment. Code reads it from there; never hard-code such values in other files. When adding a new feature, put its tunable values in that file. If a value must also appear in `platformio.ini` (e.g. OTA upload host/password), note the duplication in both places.
- EEZ Studio project work (screens, widgets, styles, fonts, flow, the `.eez-project` JSON): use the project skill in `.claude/skills/eezstudio/SKILL.md`. Its top "READ FIRST" section adapts it to this project (PlatformIO, `src/ui`, LVGL 9.5, flow enabled) and overrides the general text where they conflict.
- Record knowledge specific to this all-in-one board (ESP32-S3, LCD touchscreen, speaker, mic, SD, battery) in `board/board.md`.
