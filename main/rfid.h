#pragma once
#include "Config.h"
#include "logger.h"
#include "buzzer.h"


void setupRFID_LCD() {
    // Khởi tạo SPI cứng với các chân tường minh (SCK=18, MISO=19, MOSI=23, SS=5)
    SPI.begin(18, 19, 23, SS_PIN);       
    Wire.begin();      
    
    // Kiểm tra kết nối I2C của LCD
    Wire.beginTransmission(0x27);
    if (Wire.endTransmission() == 0) {
        SystemLog("LCD", "Kết nối LCD I2C thành công.");
    } else {
        SystemLog("LCD", "Không tìm thấy LCD I2C.");
    }
    
    mfrc522.PCD_Init(); 
    
    // Kiểm tra kết nối với RC522 bằng cách đọc version
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    if (v == 0x00 || v == 0xFF) {
        SystemLog("RFID", "Không tìm thấy module RC522.");
    } else {
        SystemLog("RFID", "Khởi tạo RC522 thành công. FW: 0x" + String(v, HEX));
    }
    
    lcd.init();         
    lcd.backlight();    
    
    lcd.setCursor(0, 0);
    lcd.print("He thong Khoa");
    lcd.setCursor(0, 1);
    lcd.print("San sang quet...");
}

void handleRFID_LCD(unsigned long currentTime) {
    // Kiểm tra có thẻ không
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return; 
    }

    // Đọc mã thẻ
    String cardID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        cardID += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        cardID += String(mfrc522.uid.uidByte[i], HEX);
    }
    cardID.trim();
    cardID.toUpperCase();

    SystemLog("RFID", "Phát hiện thẻ: " + cardID);

    lcd.clear(); 
    lcd.setCursor(0, 0); lcd.print("Ma the cua ban:");
    lcd.setCursor(0, 1); lcd.print(cardID);
    
    isShowingMessage = true;
    lcdMessageTime = currentTime;

    // Gửi qua bộ quản lý 2FA
    authManager.submitRFID(cardID);

    mfrc522.PICC_HaltA();
}