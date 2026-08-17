#include "2fa.h"
#include "Config.h"
#include "buzzer.h"
#include "logger.h"
#include "mqtt_client.h"
#include <ArduinoJson.h>

Authenticator authManager; // Khởi tạo biến cục bộ toàn cục

#include "bluetooth.h"
#include "fingerprint.h"
#include "keypad.h"
#include "rfid.h"

unsigned long lcdMessageTime = 0;
bool isShowingMessage = false;
unsigned long lastActionTime = 0;
void setup() {
  Serial.begin(115200);
  SystemLog("MAIN", "Khởi động hệ thống.");
  setupBuzzerAndLEDs();
  setupRFID_LCD();
  setupKeypad();
  setupFingerprint();
  setupBluetooth(); 
  setup_wifi();

  SystemLog("MAIN", "Hoàn tất khởi tạo.");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastActionTime >= 1000) {
    lastActionTime = currentTime;
  }

  static unsigned long lastMqttReconnectAttempt = 0;
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()) {
      if (currentTime - lastMqttReconnectAttempt > 5000) {
        lastMqttReconnectAttempt = currentTime;
        mqttReconnect();
      }
    } else {
      mqttClient.loop();
    }
  }

  if (isShowingMessage && (currentTime - lcdMessageTime >= 2000)) {
    isShowingMessage = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("He thong Khoa");
    lcd.setCursor(0, 1);
    lcd.print("San sang quet...");
  }

  handleKeypad(currentTime);
  handleRFID_LCD(currentTime);
  handleFingerprint(currentTime);
}