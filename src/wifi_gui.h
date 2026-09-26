#pragma once

// Glue between wifi_manager and the EEZ Wifi / WifiSaved screens.
void wifi_gui_init();  // call after ui_init()
void wifi_gui_loop();  // call from loop()
