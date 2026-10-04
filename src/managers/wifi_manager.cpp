#include "managers/wifi_manager.h"
#include <WiFi.h>
#include <Preferences.h>
#include "app_config.h"

WifiManager wifi_manager;

enum State { IDLE, SCANNING, CONNECTING, CONNECTED, FAILED };

static State s_state = IDLE;
static String s_ssid[WIFI_SAVED_MAX], s_pass[WIFI_SAVED_MAX];
static String s_pending_ssid, s_pending_pass;
static uint32_t s_connect_start;
static uint32_t s_scan_start;
static ScanResult s_scan_result = ScanResult::NONE;
static String s_scan_ssids;
static int s_autoconnect_next = -1;  // next saved slot to try at boot, -1 = done
static const String s_empty;

static void save_list() {
  Preferences p;
  p.begin("wifi", false);
  for (int i = 0; i < WIFI_SAVED_MAX; i++) {
    p.putString(("ssid" + String(i)).c_str(), s_ssid[i]);
    p.putString(("pass" + String(i)).c_str(), s_pass[i]);
  }
  p.end();
}

static void load_list() {
  Preferences p;
  p.begin("wifi", true);
  for (int i = 0; i < WIFI_SAVED_MAX; i++) {
    s_ssid[i] = p.getString(("ssid" + String(i)).c_str(), "");
    s_pass[i] = p.getString(("pass" + String(i)).c_str(), "");
  }
  p.end();
}

// Move/insert a network to slot 0 (most recent), dropping the oldest when full
static void remember(const String &ssid, const String &pass) {
  int found = WIFI_SAVED_MAX - 1;
  for (int i = 0; i < WIFI_SAVED_MAX; i++) {
    if (s_ssid[i] == ssid) { found = i; break; }
  }
  for (int i = found; i > 0; i--) {
    s_ssid[i] = s_ssid[i - 1];
    s_pass[i] = s_pass[i - 1];
  }
  s_ssid[0] = ssid;
  s_pass[0] = pass;
  save_list();
}

static void begin_connect(const String &ssid, const String &pass) {
  WiFi.disconnect();
  s_pending_ssid = ssid;
  s_pending_pass = pass;
  s_connect_start = millis();
  s_state = CONNECTING;
  WiFi.begin(ssid.c_str(), pass.c_str());
}

static void try_next_saved() {
  while (s_autoconnect_next >= 0 && s_autoconnect_next < WIFI_SAVED_MAX) {
    const int i = s_autoconnect_next++;
    if (s_ssid[i].length()) {
      begin_connect(s_ssid[i], s_pass[i]);
      return;
    }
  }
  s_autoconnect_next = -1;
}

void WifiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI_HOSTNAME);
  WiFi.setAutoReconnect(true);
  load_list();
#if WIFI_AUTOCONNECT_AT_BOOT
  s_autoconnect_next = 0;
  try_next_saved();
#endif
}

void WifiManager::loop() {
  if (s_state == SCANNING) {
    const int n = WiFi.scanComplete();
    if (n >= 0) {
      // Strongest first (scan results are sorted by RSSI), skip hidden and duplicate SSIDs
      String opts;
      for (int i = 0; i < n; i++) {
        const String ssid = WiFi.SSID(i);
        if (!ssid.length() || ("\n" + opts + "\n").indexOf("\n" + ssid + "\n") >= 0) continue;
        if (opts.length()) opts += "\n";
        opts += ssid;
      }
      WiFi.scanDelete();
      s_scan_ssids = opts;
      s_scan_result = ScanResult::DONE;
      s_state = WiFi.isConnected() ? CONNECTED : IDLE;
    } else if (n == WIFI_SCAN_FAILED && millis() - s_scan_start > WIFI_SCAN_TIMEOUT_MS) {
      // The core reports FAILED after its own 6 s timeout, but on this board a full scan takes ~6.4 s.
      // Keep waiting: when SCAN_DONE arrives the core resets its timer and returns the real count.
      s_scan_ssids = "";
      s_scan_result = ScanResult::FAILED;
      s_state = WiFi.isConnected() ? CONNECTED : IDLE;
    }
    return;
  }

  if (s_state == CONNECTING) {
    if (WiFi.isConnected()) {
      s_state = CONNECTED;
      s_autoconnect_next = -1;
      remember(s_pending_ssid, s_pending_pass);
    } else if (millis() - s_connect_start > WIFI_CONNECT_TIMEOUT_MS) {
      WiFi.disconnect();
      s_state = FAILED;
      try_next_saved();  // at boot, fall through to the next saved network
    }
    return;
  }

  if (s_state == CONNECTED && !WiFi.isConnected()) s_state = IDLE;
}

void WifiManager::scan_start() {
  if (s_state == CONNECTING) WiFi.disconnect();  // a pending attempt blocks scanning
  s_autoconnect_next = -1;
  s_scan_result = ScanResult::NONE;
  WiFi.scanNetworks(true);
  s_scan_start = millis();
  s_state = SCANNING;
}

ScanResult WifiManager::scan_take(String &ssids) {
  const ScanResult r = s_scan_result;
  s_scan_result = ScanResult::NONE;
  ssids = s_scan_ssids;
  return r;
}

void WifiManager::connect(const String &ssid, const String &pass) {
  if (!ssid.length()) return;
  String p = pass;
  if (!p.length()) {
    for (int i = 0; i < WIFI_SAVED_MAX; i++) {
      if (s_ssid[i] == ssid) { p = s_pass[i]; break; }
    }
  }
  s_autoconnect_next = -1;
  begin_connect(ssid, p);
}

void WifiManager::disconnect() {
  s_autoconnect_next = -1;
  WiFi.disconnect();
  s_state = IDLE;
}

void WifiManager::forget(int index) {
  if (index < 0 || index >= WIFI_SAVED_MAX || !s_ssid[index].length()) return;
  for (int i = index; i < WIFI_SAVED_MAX - 1; i++) {
    s_ssid[i] = s_ssid[i + 1];
    s_pass[i] = s_pass[i + 1];
  }
  s_ssid[WIFI_SAVED_MAX - 1] = "";
  s_pass[WIFI_SAVED_MAX - 1] = "";
  save_list();
}

const String &WifiManager::saved_ssid(int index) const {
  return (index >= 0 && index < WIFI_SAVED_MAX) ? s_ssid[index] : s_empty;
}

bool WifiManager::connected() const { return WiFi.isConnected(); }

const char *WifiManager::status_text() const {
  switch (s_state) {
    case SCANNING:   return "Scanning...";
    case CONNECTING: return "Connecting...";
    case CONNECTED:  return "Connected";
    case FAILED:     return "Connect failed";
    default:         return "Disconnected";
  }
}

String WifiManager::ssid_text() const {
  if (s_state == CONNECTING) return s_pending_ssid;
  return WiFi.isConnected() ? WiFi.SSID() : String("-");
}

String WifiManager::ip_text() const { return WiFi.isConnected() ? WiFi.localIP().toString() : String("-"); }
String WifiManager::rssi_text() const { return WiFi.isConnected() ? String(WiFi.RSSI()) + " dBm" : String("-"); }
String WifiManager::mac_text() const { return WiFi.macAddress(); }
