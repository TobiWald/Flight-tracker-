#pragma once
#include <Arduino.h>

void displayInit();
void displayWifiSetup();
void displayMessage(const String& line1, const String& line2, uint32_t color);
