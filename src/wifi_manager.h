#pragma once
#include <Arduino.h>

// WiFi station control with up to WIFI_SAVED_MAX saved networks in NVS (most recent first).
// No LVGL here; the GUI side lives in wifi_gui.cpp.

void wifi_init();                                  // load saved networks, optional auto-connect
void wifi_loop();                                  // call often; drives connect timeout + saving
void wifi_scan_start();                            // async scan
bool wifi_scan_take(String &options);              // true once when a scan finishes; '\n'-separated SSIDs
void wifi_connect(const String &ssid, const String &pass);  // empty pass + saved SSID = use saved password
void wifi_disconnect();
void wifi_forget(int index);

const String &wifi_saved_ssid(int index);          // "" when the slot is empty
const char *wifi_status_text();
String wifi_ssid_text();
String wifi_ip_text();
String wifi_rssi_text();
String wifi_mac_text();
