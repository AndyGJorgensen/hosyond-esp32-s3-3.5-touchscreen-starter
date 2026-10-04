#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_WIFI = 2,
    SCREEN_ID_WIFI_SAVED = 3,
    _SCREEN_ID_LAST = 3
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *wifi;
    lv_obj_t *wifi_saved;
    lv_obj_t *card_wifi;
    lv_obj_t *main_wifi_btn;
    lv_obj_t *card_battery;
    lv_obj_t *card_sd;
    lv_obj_t *card_speaker;
    lv_obj_t *play_sd_btn;
    lv_obj_t *card_mic;
    lv_obj_t *mic_level_bar;
    lv_obj_t *card_display;
    lv_obj_t *backlight_slider;
    lv_obj_t *led_btn;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *wifi_network;
    lv_obj_t *wifi_password;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *ota_switch;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *wifi_keyboard;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *obj24;
    lv_obj_t *obj25;
    lv_obj_t *obj26;
    lv_obj_t *obj27;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_screen_wifi();
void tick_screen_wifi();

void create_screen_wifi_saved();
void tick_screen_wifi_saved();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/