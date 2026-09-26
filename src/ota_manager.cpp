#include "ota_manager.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "app_config.h"

static bool s_enabled;
static bool s_running;  // ArduinoOTA.begin() has been called
static char s_status[32] = "Off";
static void (*s_progress_hook)();

static void setup_callbacks() {
  static bool done;
  if (done) return;
  done = true;
  ArduinoOTA.setHostname(WIFI_HOSTNAME);
  ArduinoOTA.setPort(OTA_PORT);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() { strlcpy(s_status, "Updating 0%", sizeof(s_status)); });
  ArduinoOTA.onProgress([](unsigned int done_bytes, unsigned int total) {
    static uint32_t last;
    if (millis() - last < 200) return;
    last = millis();
    snprintf(s_status, sizeof(s_status), "Updating %u%%", total ? done_bytes * 100 / total : 0);
    if (s_progress_hook) s_progress_hook();
  });
  ArduinoOTA.onEnd([]() {
    strlcpy(s_status, "Done, restarting", sizeof(s_status));
    if (s_progress_hook) s_progress_hook();
  });
  ArduinoOTA.onError([](ota_error_t err) {
    const char *why = err == OTA_AUTH_ERROR ? "auth" : err == OTA_BEGIN_ERROR ? "begin"
                    : err == OTA_CONNECT_ERROR ? "connect" : err == OTA_RECEIVE_ERROR ? "receive" : "end";
    snprintf(s_status, sizeof(s_status), "Error: %s", why);
  });
}

void ota_set_enabled(bool enabled) {
  s_enabled = enabled;
  if (!enabled && s_running) {
    ArduinoOTA.end();
    s_running = false;
  }
  if (!enabled) strlcpy(s_status, "Off", sizeof(s_status));
}

bool ota_enabled() { return s_enabled; }

void ota_set_progress_hook(void (*hook)()) { s_progress_hook = hook; }

void ota_loop() {
  if (!s_enabled) return;
  const bool wifi = WiFi.isConnected();
  if (wifi && !s_running) {
    setup_callbacks();
    ArduinoOTA.begin();
    s_running = true;
    strlcpy(s_status, "Ready", sizeof(s_status));
  } else if (!wifi && s_running) {
    ArduinoOTA.end();
    s_running = false;
  }
  if (!s_running) {
    strlcpy(s_status, "Waiting for WiFi", sizeof(s_status));
    return;
  }
  ArduinoOTA.handle();  // blocks for the whole update when one arrives
}

const char *ota_status_text() { return s_status; }
