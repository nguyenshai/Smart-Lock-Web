#pragma once
#include <Arduino.h>

void SystemLog(String source, String message) {
    unsigned long t = millis();
    
    unsigned long ms = t % 1000;
    unsigned long total_seconds = t / 1000;
    unsigned long seconds = total_seconds % 60;
    unsigned long minutes = (total_seconds / 60) % 60;

    char timeStr[20];
    sprintf(timeStr, "[%02lu:%02lu.%03lu] ", minutes, seconds, ms);
    
    Serial.print(timeStr);
    Serial.print(source);
    Serial.print(": ");
    Serial.println(message);
}