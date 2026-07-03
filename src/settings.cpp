#include "settings.h"
#include <Preferences.h>

namespace {
constexpr const char *kNamespace = "m5hid";
constexpr const char *kUsbMode = "usb";
constexpr const char *kMouseMode = "mouse";
constexpr const char *kMouseSpeed = "speed";
}  // namespace

void settingsLoad(AppSettings &settings) {
    Preferences prefs;
    if (!prefs.begin(kNamespace, true)) {
        return;
    }

    settings.usbMode = prefs.getBool(kUsbMode, settings.usbMode);
    settings.mouseMode = prefs.getBool(kMouseMode, settings.mouseMode);
    settings.mouseSpeed = prefs.getUChar(kMouseSpeed, settings.mouseSpeed);

    if (settings.mouseSpeed < 1) {
        settings.mouseSpeed = 1;
    } else if (settings.mouseSpeed > 10) {
        settings.mouseSpeed = 10;
    }

    prefs.end();
}

void settingsSave(const AppSettings &settings) {
    Preferences prefs;
    if (!prefs.begin(kNamespace, false)) {
        return;
    }

    prefs.putBool(kUsbMode, settings.usbMode);
    prefs.putBool(kMouseMode, settings.mouseMode);
    prefs.putUChar(kMouseSpeed, settings.mouseSpeed);
    prefs.end();
}
