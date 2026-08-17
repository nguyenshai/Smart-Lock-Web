#pragma once
#include "Config.h"
#include "logger.h"
#include "buzzer.h"

extern void mqttPublishVerify(String type, String value);
extern void mqttPublishAlert(String reason);

class Authenticator {
private:
    void showWaitScreen(String method) {
        SystemLog("AUTH", "Đang gửi xác thực (" + method + ")...");
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Dang kiem tra...");
        lcd.setCursor(0, 1); lcd.print("Vui long doi!");
        
        isShowingMessage = true;
        lcdMessageTime = millis();
    }

    bool checkLockdown() {
        if (isLockdown) {
            SystemLog("AUTH", "Hệ thống khóa chết, từ chối giao dịch.");
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("DANG KHOA CHET");
            lcd.setCursor(0, 1); lcd.print("Tu choi truy cap");
            isShowingMessage = true;
            lcdMessageTime = millis();
            actionDenied();
            
            // Gửi cảnh báo lên Telegram (Node-RED xử lý)
            mqttPublishAlert("Cố gắng mở cửa trong lúc KHOA CHET");
            return true;
        }
        return false;
    }

public:
    void submitPassword(String password) {
        if (checkLockdown()) return;
        showWaitScreen("PIN");
        mqttPublishVerify("pin", password);
    }

    void submitRFID(String cardID) {
        if (checkLockdown()) return;
        showWaitScreen("RFID");
        mqttPublishVerify("rfid", cardID);
    }

    void submitFingerprint(int fingerID) {
        if (checkLockdown()) return;
        showWaitScreen("FINGERPRINT");
        mqttPublishVerify("fingerprint", String(fingerID));
    }

    void submitBluetooth(String deviceID) {
        if (checkLockdown()) return;
        showWaitScreen("BLUETOOTH");
        mqttPublishVerify("ble", deviceID);
    }
};

extern Authenticator authManager;
