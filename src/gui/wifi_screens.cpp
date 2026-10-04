// Native actions and variables for the Wifi / WifiSaved EEZ screens (WiFi + OTA).
// Only content, state and flags are changed on objects.* here; layout stays in the .eez-project.
#include "gui/wifi_screens.h"
#include <lvgl.h>
#include "app_config.h"
#include "managers/wifi_manager.h"
#include "managers/ota_manager.h"
#include "ui/ui.h"
#include "ui/screens.h"
#include "ui/actions.h"
#include "ui/vars.h"

// Buffers returned by the native variable getters (EEZ flow reads them every tick)
static char s_status[40], s_ssid[40], s_ip[20], s_rssi[16], s_mac[20], s_ota[24];
static char s_saved[WIFI_SAVED_MAX][40];
static bool s_has_networks;  // the Network dropdown holds scan results (not a placeholder like "Press Scan")

static void copy(char *dst, size_t n, const String &src) { strlcpy(dst, src.c_str(), n); }

static void refresh_values() {
  strlcpy(s_status, wifi_manager.status_text(), sizeof(s_status));
  copy(s_ssid, sizeof(s_ssid), wifi_manager.ssid_text());
  copy(s_ip, sizeof(s_ip), wifi_manager.ip_text());
  copy(s_rssi, sizeof(s_rssi), wifi_manager.rssi_text());
  copy(s_mac, sizeof(s_mac), wifi_manager.mac_text());
  strlcpy(s_ota, ota_manager.status_text(), sizeof(s_ota));
  for (int i = 0; i < WIFI_SAVED_MAX; i++) {
    const String &ssid = wifi_manager.saved_ssid(i);
    copy(s_saved[i], sizeof(s_saved[i]), ssid.length() ? ssid : String("(empty)"));
  }
}

// An OTA update blocks loop(); keep the screen alive so the OTA row shows progress
static void ota_progress() {
  refresh_values();
  ui_tick();
  lv_timer_handler();
}

void wifi_screens_init() {
  ota_manager.set_progress_hook(ota_progress);
  if (ota_manager.enabled() && objects.ota_switch) lv_obj_add_state(objects.ota_switch, LV_STATE_CHECKED);
  refresh_values();
}

void wifi_screens_loop() {
  String ssids;
  const ScanResult r = wifi_manager.scan_take(ssids);
  if (r != ScanResult::NONE && objects.wifi_network) {
    s_has_networks = r == ScanResult::DONE && ssids.length();
    lv_dropdown_set_options(objects.wifi_network, s_has_networks ? ssids.c_str()
                                                  : r == ScanResult::FAILED ? "Scan failed" : "No networks found");
  }

  static uint32_t last;
  if (millis() - last >= 250) {
    last = millis();
    refresh_values();
  }
}

static int user_data(lv_event_t *e) { return (int)(intptr_t)lv_event_get_user_data(e); }

// ---- Actions ----

void action_nav(lv_event_t *e) {
  eez_flow_set_screen(user_data(e), LV_SCR_LOAD_ANIM_NONE, 0, 0);
}

void action_wifi_scan(lv_event_t *e) {
  s_has_networks = false;
  if (objects.wifi_network) lv_dropdown_set_options(objects.wifi_network, "Scanning...");
  wifi_manager.scan_start();
}

void action_wifi_connect(lv_event_t *e) {
  if (!s_has_networks || !objects.wifi_network || !objects.wifi_password) return;
  char ssid[64];
  lv_dropdown_get_selected_str(objects.wifi_network, ssid, sizeof(ssid));
  wifi_manager.connect(ssid, lv_textarea_get_text(objects.wifi_password));
  if (objects.wifi_keyboard) lv_obj_add_flag(objects.wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
}

void action_wifi_disconnect(lv_event_t *e) { wifi_manager.disconnect(); }

void action_ota_toggle(lv_event_t *e) {
  ota_manager.set_enabled(lv_obj_has_state((lv_obj_t *)lv_event_get_target(e), LV_STATE_CHECKED));
}

void action_wifi_pw_open(lv_event_t *e) {
  if (objects.wifi_keyboard) lv_obj_remove_flag(objects.wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
}

void action_wifi_kb_close(lv_event_t *e) {
  if (objects.wifi_keyboard) lv_obj_add_flag(objects.wifi_keyboard, LV_OBJ_FLAG_HIDDEN);
  if (objects.wifi_password) lv_obj_remove_state(objects.wifi_password, LV_STATE_FOCUSED);
}

void action_saved_connect(lv_event_t *e) {
  const String &ssid = wifi_manager.saved_ssid(user_data(e));
  if (ssid.length()) wifi_manager.connect(ssid, "");  // empty password = use the saved one
}

void action_saved_forget(lv_event_t *e) { wifi_manager.forget(user_data(e)); }

// ---- Native variables (read-only from the UI; setters are unused) ----

bool get_var_wifi_connected() { return wifi_manager.connected(); }  // swaps Connect/Disconnect
void set_var_wifi_connected(bool) {}
const char *get_var_wifi_status() { return s_status; }
void set_var_wifi_status(const char *) {}
const char *get_var_wifi_ssid() { return s_ssid; }
void set_var_wifi_ssid(const char *) {}
const char *get_var_wifi_ip() { return s_ip; }
void set_var_wifi_ip(const char *) {}
const char *get_var_wifi_hostname() { return WIFI_HOSTNAME; }
void set_var_wifi_hostname(const char *) {}
const char *get_var_wifi_rssi() { return s_rssi; }
void set_var_wifi_rssi(const char *) {}
const char *get_var_wifi_mac() { return s_mac; }
void set_var_wifi_mac(const char *) {}
const char *get_var_ota_status() { return s_ota; }
void set_var_ota_status(const char *) {}
const char *get_var_saved_wifi_1() { return s_saved[0]; }
void set_var_saved_wifi_1(const char *) {}
const char *get_var_saved_wifi_2() { return s_saved[1]; }
void set_var_saved_wifi_2(const char *) {}
const char *get_var_saved_wifi_3() { return s_saved[2]; }
void set_var_saved_wifi_3(const char *) {}
