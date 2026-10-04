#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_NONE
};

// Native global variables

extern const char *get_var_wifi_status();
extern void set_var_wifi_status(const char *value);
extern const char *get_var_wifi_ssid();
extern void set_var_wifi_ssid(const char *value);
extern const char *get_var_wifi_ip();
extern void set_var_wifi_ip(const char *value);
extern const char *get_var_wifi_hostname();
extern void set_var_wifi_hostname(const char *value);
extern const char *get_var_wifi_rssi();
extern void set_var_wifi_rssi(const char *value);
extern const char *get_var_wifi_mac();
extern void set_var_wifi_mac(const char *value);
extern const char *get_var_ota_status();
extern void set_var_ota_status(const char *value);
extern const char *get_var_saved_wifi_1();
extern void set_var_saved_wifi_1(const char *value);
extern const char *get_var_saved_wifi_2();
extern void set_var_saved_wifi_2(const char *value);
extern const char *get_var_saved_wifi_3();
extern void set_var_saved_wifi_3(const char *value);
extern bool get_var_wifi_connected();
extern void set_var_wifi_connected(bool value);
extern const char *get_var_sd_status();
extern void set_var_sd_status(const char *value);
extern int32_t get_var_mic_level();
extern void set_var_mic_level(int32_t value);
extern const char *get_var_battery_volts();
extern void set_var_battery_volts(const char *value);
extern const char *get_var_sd_space();
extern void set_var_sd_space(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/