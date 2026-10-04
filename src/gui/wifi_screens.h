#pragma once

// Glue between the Wifi / WifiSaved EEZ screens and wifi_manager / ota_manager.
void wifi_screens_init();  // after ui_init() and the managers' begin()
void wifi_screens_loop();  // from loop()
