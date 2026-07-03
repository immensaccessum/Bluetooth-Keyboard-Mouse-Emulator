#include "usbHid.h"

#include "hid_keys.h"

USBHIDMouse mouse;
USBHIDKeyboard keyboard;

void handleUsbMode(bool mouseMode, uint8_t mouseSpeed) {
    if (mouseMode) {
        usbMouse(mouseSpeed);
    } else {
        usbKeyboard();
    }
    delay(5);
}

void usbMouse(uint8_t mouseSpeed) {
    mouse.begin();

    if (!M5Cardputer.Keyboard.isPressed()) {
        mouse.release(MOUSE_BUTTON_LEFT);
        mouse.release(MOUSE_BUTTON_RIGHT);
        return;
    }

    const MouseDelta delta = readMouseInput(mouseSpeed);

    if (delta.buttons & 0x01) {
        mouse.press(MOUSE_BUTTON_LEFT);
    } else {
        mouse.release(MOUSE_BUTTON_LEFT);
    }

    if (delta.buttons & 0x02) {
        mouse.press(MOUSE_BUTTON_RIGHT);
    } else {
        mouse.release(MOUSE_BUTTON_RIGHT);
    }

    if (delta.x != 0 || delta.y != 0) {
        mouse.move(delta.x, delta.y);
    }
}

void usbKeyboard() {
    static bool inited = false;
    if (!inited) {
        keyboard.begin();
        inited = true;
    }

    if (!M5Cardputer.Keyboard.isChange()) {
        return;
    }

    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    KeyReport report = {0};
    fillKeyboardModifiers(report.modifiers, status);
    fillKeyboardKeycodes(report.keys, status);

    if (report.modifiers == 0) {
        bool empty = true;
        for (uint8_t key : report.keys) {
            if (key != 0) {
                empty = false;
                break;
            }
        }
        if (empty) {
            keyboard.releaseAll();
            return;
        }
    }

    keyboard.sendReport(&report);
}
