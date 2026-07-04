#include "hid_keys.h"

namespace {
constexpr uint8_t kHidSpace = 0x2C;
constexpr uint8_t kHidEnter = 0x28;
constexpr uint8_t kHidBackspace = 0x2A;
constexpr uint8_t kHidDelete = 0x4C;
constexpr uint8_t kHidTab = 0x2B;
constexpr uint8_t kHidEscape = 0x29;
constexpr uint8_t kHidUp = 0x52;
constexpr uint8_t kHidDown = 0x51;
constexpr uint8_t kHidLeft = 0x50;
constexpr uint8_t kHidRight = 0x4F;
constexpr uint8_t kHidFn = 0xFF;

bool containsKey(const uint8_t *keys, uint8_t count, uint8_t code) {
    for (uint8_t i = 0; i < count; ++i) {
        if (keys[i] == code) {
            return true;
        }
    }
    return false;
}

void appendKey(uint8_t keycode[6], uint8_t &count, uint8_t code) {
    if (containsKey(keycode, count, code) || count >= 6) {
        return;
    }
    keycode[count++] = code;
}

bool isReportableHidKey(uint8_t code) {
    if (code == 0 || code == kHidFn) {
        return false;
    }
    // Modifier keys belong in the modifier byte, not the key array.
    if (code >= 0x80 && code <= 0x87) {
        return false;
    }
    return true;
}
}  // namespace

void fillKeyboardModifiers(uint8_t &modifier, const Keyboard_Class::KeysState &status) {
    modifier = status.modifiers;
    if (status.opt) {
        modifier |= 0x08;
    }
}

void fillKeyboardKeycodes(uint8_t keycode[6], const Keyboard_Class::KeysState &status) {
    uint8_t count = 0;
    for (auto key : status.hid_keys) {
        if (isReportableHidKey(key)) {
            appendKey(keycode, count, key);
        }
    }

    if (status.tab) {
        appendKey(keycode, count, kHidTab);
    }
    if (status.backspace) {
        appendKey(keycode, count, kHidBackspace);
    }
    if (status.del) {
        appendKey(keycode, count, kHidDelete);
    }
    if (status.esc) {
        appendKey(keycode, count, kHidEscape);
    }
    if (status.up) {
        appendKey(keycode, count, kHidUp);
    }
    if (status.down) {
        appendKey(keycode, count, kHidDown);
    }
    if (status.left) {
        appendKey(keycode, count, kHidLeft);
    }
    if (status.right) {
        appendKey(keycode, count, kHidRight);
    }
    if (status.space) {
        appendKey(keycode, count, kHidSpace);
    }
    if (status.enter) {
        appendKey(keycode, count, kHidEnter);
    }
}

void applyMouseRotation(int16_t &dx, int16_t &dy, uint8_t rotation) {
    switch (rotation & 0x03) {
        case 1: {
            const int16_t tx = dx;
            dx = dy;
            dy = -tx;
            break;
        }
        case 2:
            dx = -dx;
            dy = -dy;
            break;
        case 3: {
            const int16_t tx = dx;
            dx = -dy;
            dy = tx;
            break;
        }
        default:
            break;
    }
}

MouseDelta readMouseInput(uint8_t speed, uint8_t rotation) {
    MouseDelta delta;
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    const int16_t step = speed;

    if (status.enter) {
        delta.buttons |= 0x01;
    }
    if (M5Cardputer.Keyboard.isKeyPressed('\\')) {
        delta.buttons |= 0x02;
    }

    // Mouse: direct keys or Fn-layer arrows (; , . / and status.up/down/left/right)
    if (M5Cardputer.Keyboard.isKeyPressed(';') || status.up) {
        delta.y -= step;
    } else if (M5Cardputer.Keyboard.isKeyPressed('.') || status.down) {
        delta.y += step;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('/') || status.right) {
        delta.x += step;
    } else if (M5Cardputer.Keyboard.isKeyPressed(',') || status.left) {
        delta.x -= step;
    }

    applyMouseRotation(delta.x, delta.y, rotation);
    return delta;
}

bool shouldRebootToLauncher(const Keyboard_Class::KeysState &status) {
    return status.ctrl && status.fn && status.esc;
}

bool adjustMouseSpeed(uint8_t &speed, const Keyboard_Class::KeysState &status, bool wasF11, bool wasF12) {
    bool changed = false;

    if (status.f11 && !wasF11 && speed > 1) {
        --speed;
        changed = true;
    }
    if (status.f12 && !wasF12 && speed < 10) {
        ++speed;
        changed = true;
    }

    return changed;
}

bool adjustMouseRotation(uint8_t &rotation, const Keyboard_Class::KeysState &status, bool wasF9, bool wasF10) {
    bool changed = false;

    if (status.f9 && !wasF9) {
        rotation = (rotation + 3) & 0x03;
        changed = true;
    }
    if (status.f10 && !wasF10) {
        rotation = (rotation + 1) & 0x03;
        changed = true;
    }

    return changed;
}
