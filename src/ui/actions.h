#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_nav(lv_event_t * e);
extern void action_wifi_scan(lv_event_t * e);
extern void action_wifi_connect(lv_event_t * e);
extern void action_wifi_disconnect(lv_event_t * e);
extern void action_ota_toggle(lv_event_t * e);
extern void action_wifi_pw_open(lv_event_t * e);
extern void action_wifi_kb_close(lv_event_t * e);
extern void action_saved_connect(lv_event_t * e);
extern void action_saved_forget(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/