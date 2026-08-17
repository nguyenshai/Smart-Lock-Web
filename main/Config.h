#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HardwareSerial.h>

// ==========================================
// CẤU HÌNH WIFI & MQTT
// ==========================================
#define WIFI_SSID "OnePlus Ace Pro y8mx"
#define WIFI_PASS "22060706"

#define MQTT_SERVER "42.112.213.93"
#define MQTT_PORT 1883
#define MQTT_TOPIC_VERIFY "home/hgm/verify"
#define MQTT_TOPIC_VERIFY_RESULT "home/hgm/verify_result"
#define MQTT_TOPIC_ALERT "home/hgm/alert"
#define MQTT_TOPIC_CMD "home/hgm/command"
// ==========================================

#include <BLEDevice.h>
#include <BLEScan.h>
#include <Adafruit_Fingerprint.h>

// ==========================================
// CẤU HÌNH CHÂN MODULE
// ==========================================
const int RELAY_PIN = 26;
const int redLedPin = 4;
const int buzzerPin = 32;

#define RST_PIN 27
#define SS_PIN  5
LiquidCrystal_I2C lcd(0x27, 16, 2); 
MFRC522 mfrc522(SS_PIN, RST_PIN);
BLEScan* pBLEScan;

// Biến quản lý màn hình LCD dùng chung
extern unsigned long lcdMessageTime;
extern bool isShowingMessage;

// ==========================================
// CẤU HÌNH CHÂN BÀN PHÍM MA TRẬN 4x4
// ==========================================
const byte rowPins[4] = {0, 2, 12, 15};
const byte colPins[4] = {13, 14, 25, 33};

// ==========================================
// BIẾN DÙNG CHUNG TOÀN HỆ THỐNG
// ==========================================
bool isDoorOpen = false; 
bool isLockdown = false;

const int rssiThreshold = -70;