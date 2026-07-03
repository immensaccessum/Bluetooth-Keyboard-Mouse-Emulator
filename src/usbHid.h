#ifndef USBHID_H
#define USBHID_H

#include "USBHIDMouse.h"
#include "USBHIDKeyboard.h"
#include <M5Cardputer.h>

void usbMouse(uint8_t mouseSpeed);
void usbKeyboard();
void handleUsbMode(bool mouseMode, uint8_t mouseSpeed);

#endif
