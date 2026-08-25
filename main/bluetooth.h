#pragma once
#include "Config.h"
#include "logger.h"
#include "buzzer.h"

unsigned long lastBLEVerifyTime = 0;
String lastBLEAddress = "";
const unsigned long BLE_SPAM_DELAY = 10000; // 10 giây
const char* HGM_SERVICE_UUID = "D0E1F2A3-B4C5-4678-9012-3456789ABCDE";

void TaskBluetooth(void *pvParameters) {
    while (true) {
        if (!mqttClient.connected()) {
            vTaskDelay(pdMS_TO_TICKS(250));
            continue;
        }
        BLEScanResults* foundDevices = pBLEScan->start(1, false);
        int count = foundDevices->getCount();
        
        for (int i = 0; i < count; i++) {
            BLEAdvertisedDevice device = foundDevices->getDevice(i);

            String foundAddress = String(device.getAddress().toString().c_str());
            foundAddress.toUpperCase();
            
            if (device.haveServiceUUID() &&
                device.isAdvertisingService(BLEUUID(HGM_SERVICE_UUID))) {
                int currentDB = device.getRSSI();
                if (currentDB >= rssiThreshold) {
                    unsigned long currentTime = millis();
                    
                    String bleIdentity = String(HGM_SERVICE_UUID);
                    if (bleIdentity != lastBLEAddress || (currentTime - lastBLEVerifyTime > BLE_SPAM_DELAY)) {
                        SystemLog("BLUETOOTH", "Phát hiện HGM BLE service: " + bleIdentity + " (MAC: " + foundAddress + ", RSSI: " + String(currentDB) + ")");
                        lastBLEAddress = bleIdentity;
                        lastBLEVerifyTime = currentTime;
                        
                        authManager.submitBluetooth(bleIdentity);
                    }
                }
            }
        }
        
        pBLEScan->clearResults(); 
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void setupBluetooth() {
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);

    SystemLog("BLUETOOTH", "Khởi tạo quét Bluetooth.");
    xTaskCreatePinnedToCore(
        TaskBluetooth, "TaskBLE", 10000, NULL, 1, NULL, 0 
    );
}
