#pragma once

// ArduinoOTA (host WIFI_HOSTNAME, port OTA_PORT, password OTA_PASSWORD), listening only while enabled and
// WiFi is connected. Starts enabled when OTA_ENABLED_DEFAULT is 1.
class OtaManager {
 public:
  void begin();
  void loop();  // handles OTA requests; blocks for the whole update when one arrives

  void set_enabled(bool enabled);
  bool enabled() const;
  const char *status_text() const;  // "Off", "Waiting for WiFi", "Ready", "Updating 42%", "Error: ..."

  // Called during an update (which blocks loop()) so the screen can keep showing progress.
  void set_progress_hook(void (*hook)());
};

extern OtaManager ota_manager;
