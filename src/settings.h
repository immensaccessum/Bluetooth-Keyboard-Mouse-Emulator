#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

struct AppSettings {
    bool usbMode = true;
    bool mouseMode = true;
    uint8_t mouseSpeed = 3;  // 1..10
};

void settingsLoad(AppSettings &settings);
void settingsSave(const AppSettings &settings);

#endif
