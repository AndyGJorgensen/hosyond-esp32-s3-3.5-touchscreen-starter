#pragma once

// ArduinoOTA, listening only while enabled and WiFi is connected.
void ota_set_enabled(bool enabled);
bool ota_enabled();
void ota_loop();                // call from loop(); handles OTA requests
const char *ota_status_text();  // "Off", "Waiting for WiFi", "Ready", "Updating 42%", "Error: ..."

// Called during an update, which blocks loop(), so the screen can keep showing progress.
void ota_set_progress_hook(void (*hook)());
