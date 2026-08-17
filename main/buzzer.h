#pragma once
#include "Config.h"
#include "logger.h"

#define BUZZER_ON LOW
#define BUZZER_OFF HIGH

void setupBuzzerAndLEDs() {
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(redLedPin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);

    // Mặc định: Đèn đỏ sáng, xanh tắt, còi tắt
    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(redLedPin, HIGH);
    digitalWrite(buzzerPin, BUZZER_OFF); 
}

// Hàm 1: Bíp 1 tiếng ngắn (Dùng khi bấm phím)
void beepKeypress() {
    digitalWrite(buzzerPin, BUZZER_ON); 
    delay(50);
    digitalWrite(buzzerPin, BUZZER_OFF);  
}

// Hàm 2: Sai Mật Khẩu / Sai Vân Tay / Thẻ Sai (Kêu 3 bíp + Đỏ chớp)
void actionDenied() {
    SystemLog("SYSTEM", "Truy cập bị từ chối.");
    for (int i = 0; i < 3; i++) {
        digitalWrite(redLedPin, LOW);  
        digitalWrite(buzzerPin, BUZZER_ON); 
        delay(100);
        digitalWrite(redLedPin, HIGH); 
        digitalWrite(buzzerPin, BUZZER_OFF);  
        delay(100);
    }
}

// Hàm 3: Đúng Mật Khẩu / Mở Cửa (Bíp giai điệu + Đèn Xanh + Mở Rơ-le)
void actionGranted() {
    SystemLog("SYSTEM", "Cửa đã mở.");
    digitalWrite(redLedPin, LOW);
    digitalWrite(RELAY_PIN, HIGH); // Rơ-le hút, Cửa mở
    isDoorOpen = true; // Cập nhật trạng thái
    
    // Giai điệu mở cửa vui tai
    digitalWrite(buzzerPin, BUZZER_ON); delay(150);
    digitalWrite(buzzerPin, BUZZER_OFF);  delay(100);
    digitalWrite(buzzerPin, BUZZER_ON); delay(300);
    digitalWrite(buzzerPin, BUZZER_OFF);
}

// Hàm 4: Khóa cửa
void actionLocked() {
    SystemLog("SYSTEM", "Cửa đã đóng.");
    digitalWrite(RELAY_PIN, LOW); // Đóng rơ-le
    digitalWrite(redLedPin, HIGH); // Đèn đỏ bật
    isDoorOpen = false; // Cập nhật trạng thái

    // Tiếng bíp đóng cửa
    digitalWrite(buzzerPin, BUZZER_ON); delay(200);
    digitalWrite(buzzerPin, BUZZER_OFF);  delay(100);
    digitalWrite(buzzerPin, BUZZER_ON); delay(200);
    digitalWrite(buzzerPin, BUZZER_OFF);
}