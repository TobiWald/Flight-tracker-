#include "display.h"
#include "lgfx_config.hpp"

static LGFX lcd;

#define COL_BG TFT_BLACK
#define COL_TEXT TFT_WHITE

void displayInit() {
  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(200);
  lcd.fillScreen(COL_BG);
}

void displayWifiSetup() {
  lcd.fillScreen(COL_BG);
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(COL_TEXT);
  lcd.setTextSize(2);
  lcd.drawString("WLAN-Setup:", 120, 100);
  lcd.drawString("FlightTracker-Setup", 120, 130);
}

void displayMessage(const String& line1, const String& line2, uint32_t color) {
  lcd.fillScreen(COL_BG);
  lcd.setTextDatum(middle_center);

  lcd.setTextColor(color);
  lcd.setTextSize(2);
  lcd.drawString(line1, 120, 100);

  if (line2.length() > 0) {
    lcd.setTextColor(COL_TEXT);
    lcd.setTextSize(2);
    lcd.drawString(line2, 120, 140);
  }
}
