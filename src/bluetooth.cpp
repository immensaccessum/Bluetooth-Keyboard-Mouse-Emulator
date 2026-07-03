#include "bluetooth.h"

#include "hid_keys.h"
#include <BLE2902.h>

namespace {
constexpr uint32_t kHidReadyTimeoutMs = 8000;
constexpr uint8_t kMaxStaleDisconnects = 6;
constexpr uint32_t kAdvRestartMs = 3000;
constexpr uint32_t kHidKickDelayMs = 400;

BLEServer *bleServer = nullptr;
BLEAdvertising *bleAdvertising = nullptr;
bool bluetoothLinked = false;
bool rejectAutoReconnectOnce = true;
unsigned long linkedAtMs = 0;
unsigned long hidKickAtMs = 0;
unsigned long lastAdvRestartMs = 0;
uint8_t staleDisconnectCount = 0;
bool pendingHidKick = false;

void enableReportNotifications(BLECharacteristic *characteristic) {
    if (characteristic == nullptr) {
        return;
    }

    auto *cccd =
        static_cast<BLE2902 *>(characteristic->getDescriptorByUUID(static_cast<uint16_t>(0x2902)));
    if (cccd != nullptr) {
        cccd->setNotifications(true);
    }
}

bool reportNotificationsEnabled(BLECharacteristic *characteristic) {
    if (characteristic == nullptr) {
        return false;
    }

    auto *cccd =
        static_cast<BLE2902 *>(characteristic->getDescriptorByUUID(static_cast<uint16_t>(0x2902)));
    return cccd != nullptr && cccd->getNotifications();
}

bool isBluetoothHidReady() {
    return reportNotificationsEnabled(mouseInput) || reportNotificationsEnabled(keyboardInput);
}

void sendEmptyReportsInternal() {
    if (mouseInput == nullptr || keyboardInput == nullptr) {
        return;
    }

    uint8_t emptyMouseReport[4] = {0, 0, 0, 0};
    mouseInput->setValue(emptyMouseReport, sizeof(emptyMouseReport));
    mouseInput->notify();

    uint8_t emptyKeyboardReport[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    keyboardInput->setValue(emptyKeyboardReport, sizeof(emptyKeyboardReport));
    keyboardInput->notify();
}

void ensureAdvertising() {
    if (bleAdvertising == nullptr) {
        return;
    }

    const uint32_t now = millis();
    if (now - lastAdvRestartMs < kAdvRestartMs) {
        return;
    }

    lastAdvRestartMs = now;
    bleAdvertising->start();
}

void scheduleHidKick() {
    pendingHidKick = true;
    hidKickAtMs = millis() + kHidKickDelayMs;
}

void processHidKick() {
    if (!pendingHidKick || bleServer == nullptr || bleServer->getConnectedCount() == 0) {
        return;
    }

    if (millis() < hidKickAtMs) {
        return;
    }

    pendingHidKick = false;
    sendEmptyReportsInternal();
}
}  // namespace

BLEHIDDevice *hid;
BLECharacteristic *mouseInput;
BLECharacteristic *keyboardInput;

void MyBLEServerCallbacks::onConnect(BLEServer *pServer) {
    if (rejectAutoReconnectOnce) {
        rejectAutoReconnectOnce = false;
        delay(200);
        pServer->disconnect(pServer->getConnId());
        return;
    }

    bluetoothLinked = true;
    linkedAtMs = millis();
    staleDisconnectCount = 0;
    scheduleHidKick();
}

void MyBLEServerCallbacks::onDisconnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) {
    (void)param;
    bluetoothLinked = false;
    linkedAtMs = 0;
    pendingHidKick = false;
    pServer->startAdvertising();
    lastAdvRestartMs = millis();
}

bool getBluetoothStatus() {
    return bluetoothLinked && isBluetoothHidReady();
}

void monitorBluetoothConnection() {
    if (bleServer == nullptr) {
        return;
    }

    processHidKick();

    if (bleServer->getConnectedCount() == 0) {
        bluetoothLinked = false;
        linkedAtMs = 0;
        pendingHidKick = false;
        ensureAdvertising();
        return;
    }

    bluetoothLinked = true;

    if (isBluetoothHidReady()) {
        linkedAtMs = 0;
        staleDisconnectCount = 0;
        pendingHidKick = false;
        return;
    }

    if (linkedAtMs == 0) {
        linkedAtMs = millis();
        return;
    }

    if (millis() - linkedAtMs < kHidReadyTimeoutMs) {
        return;
    }

    if (staleDisconnectCount >= kMaxStaleDisconnects) {
        return;
    }

    staleDisconnectCount++;
    linkedAtMs = 0;
    pendingHidKick = false;
    bleServer->disconnect(bleServer->getConnId());
    ensureAdvertising();
}

void bluetoothMouse(uint8_t mouseSpeed, uint8_t mouseRotation) {
    const MouseDelta delta = readMouseInput(mouseSpeed, mouseRotation);
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
    sendEmptyReportsInternal();
}

void handleBluetoothMode(bool mouseMode, uint8_t mouseSpeed, uint8_t mouseRotation) {
    monitorBluetoothConnection();

    if (!getBluetoothStatus()) {
        delay(7);
        return;
    }

    if (M5Cardputer.Keyboard.isPressed()) {
        if (mouseMode) {
            bluetoothMouse(mouseSpeed, mouseRotation);
        } else {
            bluetoothKeyboard();
        }
    } else {
        sendEmptyReports();
    }

    delay(7);
}

void initBluetooth() {
    rejectAutoReconnectOnce = true;
    staleDisconnectCount = 0;
    pendingHidKick = false;
    lastAdvRestartMs = 0;

    BLEDevice::init("M5-Keyboard-Mouse");
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new MyBLEServerCallbacks());

    hid = new BLEHIDDevice(bleServer);
    mouseInput = hid->inputReport(1);
    keyboardInput = hid->inputReport(2);
    enableReportNotifications(mouseInput);
    enableReportNotifications(keyboardInput);

    hid->manufacturer()->setValue("M5Stack");
    hid->pnp(0x02, 0x1234, 0x5678, 0x0100);
    hid->hidInfo(0x00, 0x01);
    hid->reportMap((uint8_t *)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));
    hid->startServices();

    bleAdvertising = bleServer->getAdvertising();
    bleAdvertising->setAppearance(HID_MOUSE);
    bleAdvertising->setScanResponse(true);
    bleAdvertising->setMinPreferred(0x06);
    bleAdvertising->setMaxPreferred(0x12);
    bleAdvertising->addServiceUUID(hid->hidService()->getUUID());
    bleAdvertising->start();
    lastAdvRestartMs = millis();

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
}

void deinitBluetooth() {
    BLEDevice::deinit();
    delay(1000);
}
