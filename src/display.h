#ifndef DISPLAY_H
#define DISPLAY_H

#include <M5Cardputer.h>

void setupDisplay();
void displayWelcomeScreen();
void displaySelectionScreen(bool usbMode);
void displayMainScreen(bool usbMode, bool mouseMode, bool bluetoothStatus, uint8_t mouseSpeed);
void displayMouseSpeed(uint8_t mouseSpeed);
void displayRebootMessage();
void modeIndicator(bool usbMode, bool bluetoothStatus);
void drawDeviceRect(bool mouseMode);

#endif
