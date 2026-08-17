#pragma once
#include "Config.h"
#include <Adafruit_Fingerprint.h>
#include "logger.h"
#include "buzzer.h"

// Dùng Serial2 của ESP32 (Chân 16 là RX, Chân 17 là TX)
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setupFingerprint() {
    mySerial.begin(57600, SERIAL_8N1, 16, 17);
    finger.begin(57600);

    if (finger.verifyPassword()) {
        SystemLog("FINGERPRINT", "Tìm thấy cảm biến vân tay AS608.");
    } else {
        SystemLog("FINGERPRINT", "Không tìm thấy cảm biến vân tay.");
    }
}

void handleFingerprint(unsigned long currentTime) {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) {
        return; 
    }
    
    // Ghi log ngay khi có người chạm tay
    SystemLog("FINGERPRINT", "Phát hiện chạm vân tay.");

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        SystemLog("FINGERPRINT", "Ảnh vân tay lỗi, vui lòng thử lại.");
        return; 
    }

    p = finger.fingerSearch();
    
    if (p == FINGERPRINT_OK) {
        authManager.submitFingerprint(finger.fingerID);
    } 
    else if (p == FINGERPRINT_NOTFOUND) {
        SystemLog("FINGERPRINT", "Vân tay sai hoặc chưa đăng ký.");
        
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("VANTAY KHONG DUNG");
        lcd.setCursor(0, 1); lcd.print("Vui long thu lai");
        
        isShowingMessage = true;
        lcdMessageTime = currentTime;
        
        actionDenied(); // Vẫn gọi báo sai trực tiếp nếu sai hẳn
    }
}