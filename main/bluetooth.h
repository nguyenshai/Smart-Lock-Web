#pragma once
#include "Config.h"
#include "logger.h"
#include "buzzer.h"

unsigned long lastBLEVerifyTime = 0;
String lastBLEUUID = "";
const unsigned long BLE_SPAM_DELAY = 10000; // 10 giây

void TaskBluetooth(void *pvParameters) {
    while (true) {
        BLEScanResults* foundDevices = pBLEScan->start(1, false);
        int count = foundDevices->getCount();
        
        for (int i = 0; i < count; i++) {
            BLEAdvertisedDevice device = foundDevices->getDevice(i);   
            if (device.haveServiceUUID()) {
                String foundUUID = String(device.getServiceUUID().toString().c_str());
                
                // Chi quet UUID 128-bit (do dai 36 ky tu)
                if (foundUUID.length() == 36) {
                    int currentDB = device.getRSSI(); 
                    if (currentDB >= rssiThreshold) {
                        unsigned long currentTime = millis();
                        
                        // Kiem tra spam
                        if (foundUUID != lastBLEUUID || (currentTime - lastBLEVerifyTime > BLE_SPAM_DELAY)) {
                            SystemLog("BLUETOOTH", "Phát hiện BLE UUID: " + foundUUID + " (RSSI: " + String(currentDB) + ")");
                            lastBLEUUID = foundUUID;
                            lastBLEVerifyTime = currentTime;
                            
                            authManager.submitBluetooth(foundUUID);
                        }
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