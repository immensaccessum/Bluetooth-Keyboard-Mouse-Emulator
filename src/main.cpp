#include <M5Cardputer.h>
#include <USB.h>
#include <esp_system.h>

#include "bluetooth.h"
#include "display.h"
#include "hid_keys.h"
#include "settings.h"
#include "usbHid.h"

AppSettings appSettings;
bool lastBluetoothStatus = false;
bool lastF11 = false;
bool lastF12 = false;
bool lastF9 = false;
bool lastF10 = false;

void selectMode() {
    bool lastMode = !appSettings.usbMode;

    while (true) {
        M5Cardputer.update();

        if (lastMode != appSettings.usbMode) {
            displaySelectionScreen(appSettings.usbMode);
            lastMode = appSettings.usbMode;
        }

        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            if (M5Cardputer.Keyboard.isKeyPressed('.') || M5Cardputer.Keyboard.isKeyPressed(';')) {
                appSettings.usbMode = !appSettings.usbMode;
            }

            Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
            if (status.enter) {
                settingsSave(appSettings);
                break;
            }
        }

        delay(10);
    }
}

void rebootToLauncher() {
    displayRebootMessage();
    delay(400);
    esp_restart();
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    settingsLoad(appSettings);

    setupDisplay();
    displayWelcomeScreen();
    selectMode();

    if (appSettings.usbMode) {
        USB.begin();
    } else {
        initBluetooth();
    }

    displayMainScreen(appSettings.usbMode, appSettings.mouseMode, getBluetoothStatus(),
                      appSettings.mouseSpeed, appSettings.mouseRotation);
}

void loop() {
    M5Cardputer.update();

    if (!appSettings.usbMode) {
        monitorBluetoothConnection();
    }

    const bool bluetoothStatus = getBluetoothStatus();
    if (lastBluetoothStatus != bluetoothStatus) {
        modeIndicator(appSettings.usbMode, bluetoothStatus);
        lastBluetoothStatus = bluetoothStatus;
    }

    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

        if (shouldRebootToLauncher(status)) {
            rebootToLauncher();
        }

        if (appSettings.mouseMode &&
            adjustMouseSpeed(appSettings.mouseSpeed, status, lastF11, lastF12)) {
            settingsSave(appSettings);
            displayMouseStatus(appSettings.mouseSpeed, appSettings.mouseRotation);
        }

        if (appSettings.mouseMode &&
            adjustMouseRotation(appSettings.mouseRotation, status, lastF9, lastF10)) {
            settingsSave(appSettings);
            displayMouseStatus(appSettings.mouseSpeed, appSettings.mouseRotation);
        }

        lastF11 = status.f11;
        lastF12 = status.f12;
        lastF9 = status.f9;
        lastF10 = status.f10;
    } else if (!M5Cardputer.Keyboard.isPressed()) {
        lastF11 = false;
        lastF12 = false;
        lastF9 = false;
        lastF10 = false;
    }

    if (M5Cardputer.BtnA.isPressed()) {
        appSettings.mouseMode = !appSettings.mouseMode;
        settingsSave(appSettings);
        drawDeviceRect(appSettings.mouseMode);
        delay(200);
    }

    if (appSettings.usbMode) {
        handleUsbMode(appSettings.mouseMode, appSettings.mouseSpeed, appSettings.mouseRotation);
    } else {
        handleBluetoothMode(appSettings.mouseMode, appSettings.mouseSpeed, appSettings.mouseRotation);
    }
}
