#pragma once
#include <Arduino.h>

// WiFi station with up to WIFI_SAVED_MAX saved networks in NVS (most recent first).
// A network is saved only after it connects. At boot (WIFI_AUTOCONNECT_AT_BOOT) the saved ones are tried in order.
enum class ScanResult : uint8_t { NONE, DONE, FAILED };

class WifiManager {
 public:
  void begin();
  void loop();  // drives scans, connect timeouts and saving

  void scan_start();               // async (a full scan takes ~6.4 s on this board)
  // Once per finished scan: DONE with '\n'-separated SSIDs, strongest first ("" = none found), or FAILED
  ScanResult scan_take(String &ssids);
  void connect(const String &ssid, const String &pass);  // empty pass + a saved SSID = use the saved password
  void disconnect();
  void forget(int index);

  bool connected() const;
  const String &saved_ssid(int index) const;  // "" when the slot is empty
  const char *status_text() const;            // "Disconnected", "Scanning...", "Connecting...", ...
  String ssid_text() const;
  String ip_text() const;
  String rssi_text() const;
  String mac_text() const;
};

extern WifiManager wifi_manager;
