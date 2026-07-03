
#include "bluetooth.h"

#include "hid_keys.h"

BLEHIDDevice *hid;
BLECharacteristic *mouseInput;
BLECharacteristic *keyboardInput;
bool bluetoothIsConnected = false;

void MyBLEServerCallbacks::onConnect(BLEServer *pServer) {
    bluetoothIsConnected = true;
}

void MyBLEServerCallbacks::onDisconnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) {
    bluetoothIsConnected = false;

    pServer->disconnect(param->disconnect.conn_id);
    pServer->startAdvertising();
}

bool getBluetoothStatus() {
    return bluetoothIsConnected;
}

void bluetoothMouse(uint8_t mouseSpeed) {
    const MouseDelta delta = readMouseInput(mouseSpeed);
    const int8_t x = constrain(delta.x, -127, 127);
    const int8_t y = constrain(delta.y, -127, 127);
    const uint8_t report[4] = {
        delta.buttons,
        static_cast<uint8_t>(x),
        static_cast<uint8_t>(y),
        0,
    };

    mouseInput->setValue(const_cast<uint8_t *>(report), sizeof(report));
    mouseInput->notify();
}

void bluetoothKeyboard() {
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

    uint8_t modifier = 0;
    uint8_t keycode[6] = {0};
    fillKeyboardModifiers(modifier, status);
    fillKeyboardKeycodes(keycode, status);

    const uint8_t report[8] = {
        modifier,
        0,
        keycode[0],
        keycode[1],
        keycode[2],
        keycode[3],
        keycode[4],
        keycode[5],
    };

    keyboardInput->setValue(const_cast<uint8_t *>(report), sizeof(report));
    keyboardInput->notify();

    delay(50);
}

void sendEmptyReports() {
    uint8_t emptyMouseReport[4] = {0, 0, 0, 0};
    mouseInput->setValue(emptyMouseReport, sizeof(emptyMouseReport));
    mouseInput->notify();

    uint8_t emptyKeyboardReport[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    keyboardInput->setValue(emptyKeyboardReport, sizeof(emptyKeyboardReport));
    keyboardInput->notify();
}

void handleBluetoothMode(bool mouseMode, uint8_t mouseSpeed) {
    if (!bluetoothIsConnected) {
        delay(7);
        return;
    }

    if (M5Cardputer.Keyboard.isPressed()) {
        if (mouseMode) {
            bluetoothMouse(mouseSpeed);
        } else {
            bluetoothKeyboard();
        }
    } else {
        sendEmptyReports();
    }

    delay(7);
}

void initBluetooth() {
    BLEDevice::init("M5-Keyboard-Mouse");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyBLEServerCallbacks());

    hid = new BLEHIDDevice(pServer);
    mouseInput = hid->inputReport(1);
    keyboardInput = hid->inputReport(2);

    hid->manufacturer()->setValue("M5Stack");
    hid->pnp(0x02, 0x1234, 0x5678, 0x0100);
    hid->hidInfo(0x00, 0x01);
    hid->reportMap((uint8_t *)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));
    hid->startServices();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->setAppearance(HID_MOUSE);
    pAdvertising->addServiceUUID(hid->hidService()->getUUID());
    pAdvertising->start();

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
}

void deinitBluetooth() {
    BLEDevice::deinit();
    delay(1000);
}
