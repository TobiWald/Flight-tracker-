#pragma once
#include <Arduino.h>

#define COLOR_GREEN   0x07E0
#define COLOR_SKYBLUE 0x867D
#define COLOR_RED     0xF800
#define COLOR_WHITE   0xFFFF

void displayInit();
void displayWifiSetup();
void displayMessage(const String& line1, const String& line2, uint32_t color);
