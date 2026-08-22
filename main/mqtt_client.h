#pragma once
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "logger.h"
#include "buzzer.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Hàm gửi cảnh báo
void mqttPublishAlert(String reason) {
    if (!mqttClient.connected()) {
        SystemLog("MQTT", "Không gửi cảnh báo: MQTT mất kết nối.");
        return;
    }

    StaticJsonDocument<200> doc;
    doc["reason"] = reason;

    String output;
    serializeJson(doc, output);

    bool sent = mqttClient.publish(MQTT_TOPIC_ALERT, output.c_str());

    if (sent) {
        SystemLog("MQTT", "Đã gửi cảnh báo: " + reason);
    } else {
        SystemLog("MQTT", "Lỗi gửi cảnh báo MQTT.");
    }
}

// Hàm gửi yêu cầu xác thực
void mqttPublishVerify(String type, String value) {
    if (mqttClient.connected()) {
        StaticJsonDocument<200> doc;
        doc["type"] = type;
        doc["value"] = value;
        String output;
        serializeJson(doc, output);
        mqttClient.publish(MQTT_TOPIC_VERIFY, output.c_str());
        SystemLog("MQTT", "Gửi yêu cầu xác thực: " + type + " - " + value);
    } else {
        SystemLog("MQTT", "Lỗi: Mất kết nối MQTT.");
        actionDenied();
    }
}

// Hàm gửi trạng thái khóa cửa
void mqttPublishState(bool isOpen) {
    if (mqttClient.connected()) {
        StaticJsonDocument<200> doc;
        doc["state"] = isOpen ? "unlocked" : "locked";
        String output;
        serializeJson(doc, output);
        mqttClient.publish("home/hgm/state_update", output.c_str());
        SystemLog("MQTT", "Đồng bộ trạng thái: " + String(isOpen ? "MỞ" : "KHÓA"));
    }
}

// Hàm callback nhận lệnh từ Server
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String messageTemp;
    for (int i = 0; i < length; i++) {
        messageTemp += (char)payload[i];
    }

    SystemLog("MQTT", "Nhận từ [" + String(topic) + "]: " + messageTemp);

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, messageTemp);

    if (error) {
        SystemLog("MQTT", "Lỗi phân tích JSON: " + String(error.c_str()));
        return;
    }

    String cmd = doc["cmd"] | "";

    if (String(topic) == MQTT_TOPIC_VERIFY_RESULT) {
        // Nhận kết quả xác thực từ Backend
        if (cmd == "GRANT") {
            SystemLog("MQTT", "Xác thực thành công.");
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("XAC THUC DUNG!");
            lcd.setCursor(0, 1); lcd.print("Dang mo cua...");
            isShowingMessage = true;
            lcdMessageTime = millis();
            actionGranted();
            mqttPublishAlert("Mở cửa sau khi xác thực thành công");
        } else if (cmd == "DENY") {
            SystemLog("MQTT", "Xác thực thất bại.");
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("SAI THONG TIN");
            lcd.setCursor(0, 1); lcd.print("Vui long thu lai");
            isShowingMessage = true;
            lcdMessageTime = millis();
            actionDenied();
        }
    } 
    else if (String(topic) == MQTT_TOPIC_CMD) {
        // Nhận lệnh điều khiển từ Frontend
        if (cmd == "UNLOCK") {
            SystemLog("MQTT", "Lệnh mở cửa từ xa.");
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("MO CUA TU XA");
            lcd.setCursor(0, 1); lcd.print("Remote Unlock");
            isShowingMessage = true;
            lcdMessageTime = millis();
            actionGranted();
        } 
        else if (cmd == "LOCK") {
            SystemLog("MQTT", "Lệnh đóng cửa từ xa.");
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("DONG CUA TU XA");
            isShowingMessage = true;
            lcdMessageTime = millis();
            actionLocked();
        }
        else if (cmd == "LOCKDOWN_ON") {
            SystemLog("MQTT", "Bật chế độ khóa chết.");
            isLockdown = true;
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("KHOA CHET BAT!");
            isShowingMessage = true;
            lcdMessageTime = millis();
            actionLocked(); // Đóng cửa ngay lập tức
        }
        else if (cmd == "LOCKDOWN_OFF") {
            SystemLog("MQTT", "Tắt chế độ khóa chết.");
            isLockdown = false;
            lcd.clear();
            lcd.setCursor(0, 0); lcd.print("KHOA CHET TAT!");
            isShowingMessage = true;
            lcdMessageTime = millis();
        }
    }
}

void setup_wifi() {
    delay(10);
    SystemLog("WIFI", "Kết nối Wi-Fi: " + String(WIFI_SSID));
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        SystemLog("WIFI", "Kết nối Wi-Fi thành công. IP: " + WiFi.localIP().toString());
    } else {
        SystemLog("WIFI", "Lỗi kết nối Wi-Fi.");
    }

    // Cấu hình MQTT
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    // Tăng kích thước buffer MQTT để nhận JSON
    mqttClient.setBufferSize(512);
}

void mqttReconnect() {
    if (WiFi.status() != WL_CONNECTED) return;

    if (!mqttClient.connected()) {
        SystemLog("MQTT", "Kết nối MQTT: " + String(MQTT_SERVER));
        String clientId = "ESP32DoorLock-" + String(random(0xffff), HEX);
        
        if (mqttClient.connect(clientId.c_str())) {
            SystemLog("MQTT", "Kết nối MQTT thành công.");
            // Đăng ký nhận lệnh từ các Topic
            mqttClient.subscribe(MQTT_TOPIC_CMD);
            mqttClient.subscribe(MQTT_TOPIC_VERIFY_RESULT);
            SystemLog("MQTT", "Đã đăng ký Topic command và verify_result");
        } else {
            SystemLog("MQTT", "Lỗi kết nối MQTT, mã lỗi: " + String(mqttClient.state()));
        }
    }
}
