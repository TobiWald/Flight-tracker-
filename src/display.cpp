#include "display.h"
#include "config.h"
#include <Arduino_GFX_Library.h>

static Arduino_DataBus *bus = new Arduino_ESP32SPI(
    LCD_PIN_DC, LCD_PIN_CS, LCD_PIN_SCK, LCD_PIN_MOSI);

static Arduino_GFX *gfx = new Arduino_ST7789(
    bus, LCD_PIN_RST, 0 /* rotation */, false /* IPS */,
    LCD_WIDTH, LCD_HEIGHT,
    LCD_COL_OFFSET1, LCD_ROW_OFFSET1,
    LCD_COL_OFFSET2, LCD_ROW_OFFSET2);

void displayInit() {
  gfx->begin();
  gfx->fillScreen(BLACK);
}

void displayWifiSetup() {
  gfx->fillScreen(BLACK);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 100);
  gfx->println("WLAN-Setup:");
  gfx->setCursor(10, 130);
  gfx->println("FlightTracker-");
  gfx->setCursor(10, 155);
  gfx->println("Setup");
}

static String germanize(const String& in) {
  String out;
  out.reserve(in.length());
  for (int i = 0; i < (int)in.length(); i++) {
    uint8_t c = (uint8_t)in[i];
    if (c == 0xC3 && i + 1 < (int)in.length()) {
      uint8_t c2 = (uint8_t)in[i + 1];
      switch (c2) {
        case 0xA4: out += "ae"; i++; continue;
        case 0xB6: out += "oe"; i++; continue;
        case 0xBC: out += "ue"; i++; continue;
        case 0x84: out += "Ae"; i++; continue;
        case 0x96: out += "Oe"; i++; continue;
        case 0x9C: out += "Ue"; i++; continue;
        case 0x9F: out += "ss"; i++; continue;
        default: break;
      }
    }
    out += (char)c;
  }
  return out;
}

static void drawWrapped(const String& rawText, int yStart, int lineHeight, uint32_t color) {
  String text = germanize(rawText);
  gfx->setTextColor(color);
  gfx->setTextSize(2);
  const int charsPerLine = 13;

  int start = 0;
  int y = yStart;
  while (start < (int)text.length()) {
    int end = start + charsPerLine;
    if (end >= (int)text.length()) {
      end = text.length();
    } else {
      int lastSpace = text.lastIndexOf(' ', end);
      if (lastSpace > start) end = lastSpace;
    }
    String lineStr = text.substring(start, end);
    lineStr.trim();

    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds(lineStr, 0, 0, &x1, &y1, &w, &h);
    int x = (LCD_WIDTH - w) / 2;
    if (x < 0) x = 0;

    gfx->setCursor(x, y);
    gfx->println(lineStr);
    y += lineHeight;
    start = end;
  }
}

void displayMessage(const String& line1, const String& line2, uint32_t color) {
  gfx->fillScreen(BLACK);
  drawWrapped(line1, 110, 22, color);
  if (line2.length() > 0) {
    drawWrapped(line2, 170, 22, WHITE);
  }
}
