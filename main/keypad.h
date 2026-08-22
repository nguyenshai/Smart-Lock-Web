#pragma once
#include "Config.h"
#include "logger.h"
#include "buzzer.h"

const byte ROW_NUM    = 4; 
const byte COLUMN_NUM = 4; 

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

bool buttonState[ROW_NUM][COLUMN_NUM];
bool lastButtonState[ROW_NUM][COLUMN_NUM];
unsigned long lastDebounceTime[ROW_NUM][COLUMN_NUM];
const unsigned long debounceTimeMs = 30;

String enteredPassword = ""; 
bool otpMode = false;

void updatePasswordLCD() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(otpMode ? "Nhap OTP:" : "Nhap Mat Khau:");
    lcd.setCursor(0, 1);
    
    for (int i = 0; i < enteredPassword.length(); i++) {
        lcd.print("*");
    }
    
    isShowingMessage = true;
    lcdMessageTime = millis();
}

void setupKeypad() {
    for (int c = 0; c < COLUMN_NUM; c++) {
        pinMode(colPins[c], INPUT_PULLUP);
    }
    for (int r = 0; r < ROW_NUM; r++) {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], HIGH);
    }
    for(int r = 0; r < ROW_NUM; r++){
        for(int c = 0; c < COLUMN_NUM; c++){
            buttonState[r][c] = HIGH;
            lastButtonState[r][c] = HIGH;
            lastDebounceTime[r][c] = 0;
        }
    }
}

void handleKeyPress(char key, unsigned long currentTime) {
    beepKeypress();
    SystemLog("KEYPAD", "Bấm phím: " + String(key));

    if (key == 'A') { // Phím ĐÓNG CỬA
        SystemLog("KEYPAD", "Bấm A: Đóng cửa.");
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("DONG CUA THU CONG");
        isShowingMessage = true;
        lcdMessageTime = currentTime;
        actionLocked();
        mqttPublishState(false); // Gửi trạng thái lên Web
    } else if (key == 'B' || key == 'C') {
        // Tắt không dùng
    } else if (key == 'D') {
        otpMode = !otpMode;
        enteredPassword = "";
        updatePasswordLCD();
    } else if (key == '*') { // Phím XÓA
        if (enteredPassword.length() > 0) {
            enteredPassword.remove(enteredPassword.length() - 1); 
            updatePasswordLCD();
        }
    } else if (key == '#') { // Phím ENTER
        SystemLog("KEYPAD", otpMode ? "Kiểm tra OTP..." : "Kiểm tra mật khẩu...");
        if (enteredPassword.length() > 0) {
            if (otpMode) {
                if (enteredPassword.length() == 6) {
                    authManager.submitOtp(enteredPassword);
                } else {
                    lcd.clear();
                    lcd.setCursor(0, 0); lcd.print("OTP PHAI 6 SO");
                    isShowingMessage = true;
                    lcdMessageTime = currentTime;
                }
            } else {
                authManager.submitPassword(enteredPassword);
            }
            enteredPassword = ""; 
            otpMode = false;
        }
    } else { // Các phím số
        if (enteredPassword.length() < 16) { 
            enteredPassword += key;
            updatePasswordLCD();
        }
    }
}

void handleKeypad(unsigned long currentTime) {
    for (int r = 0; r < ROW_NUM; r++) {
        digitalWrite(rowPins[r], LOW);
        
        for (int c = 0; c < COLUMN_NUM; c++) {
            bool currentState = digitalRead(colPins[c]);
            
            if (currentState != lastButtonState[r][c]) {
                lastDebounceTime[r][c] = currentTime;
            }
            
            if ((currentTime - lastDebounceTime[r][c]) > debounceTimeMs) {
                if (currentState != buttonState[r][c]) {
                    buttonState[r][c] = currentState;
                    
                    if (buttonState[r][c] == LOW) {
                        handleKeyPress(keys[r][c], currentTime);
                    }
                }
            }
            lastButtonState[r][c] = currentState;
        }
        
        digitalWrite(rowPins[r], HIGH);
    }
}