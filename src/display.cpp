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

// Ersetzt UTF-8-codierte deutsche Umlaute durch ASCII, da der Standard-Font
// von LovyanGFX keine Umlaute darstellen kann (zeigt sonst Zeichensalat)
static String germanize(const String& in) {
  String out;
  out.reserve(in.length());
  for (int i = 0; i < (int)in.length(); i++) {
    uint8_t c = (uint8_t)in[i];
    if (c == 0xC3 && i + 1 < (int)in.length()) {
      uint8_t c2 = (uint8_t)in[i + 1];
      switch (c2) {
        case 0xA4: out += "ae"; i++; continue; // ä
        case 0xB6: out += "oe"; i++; continue; // ö
        case 0xBC: out += "ue"; i++; continue; // ü
        case 0x84: out += "Ae"; i++; continue; // Ä
        case 0x96: out += "Oe"; i++; continue; // Ö
        case 0x9C: out += "Ue"; i++; continue; // Ü
        case 0x9F: out += "ss"; i++; continue; // ß
        default: break;
      }
    }
    out += (char)c;
  }
  return out;
}

// Bricht Text (nach Umlaut-Ersetzung) auf mehrere zentrierte Zeilen um,
// damit nichts über den Rand des runden Displays hinausläuft
static void drawWrappedCentered(const String& rawText, int yCenter, int lineHeight, uint32_t color) {
  String text = germanize(rawText);

  lcd.setTextDatum(middle_center);
  lcd.setTextColor(color);
  lcd.setTextSize(2);
  const int charsPerLine = 15;

  String lines[3];
  int lineCount = 0;
  int start = 0;
  while (start < (int)text.length() && lineCount < 3) {
    int end = start + charsPerLine;
    if (end >= (int)text.length()) {
      end = text.length();
    } else {
      int lastSpace = text.lastIndexOf(' ', end);
      if (lastSpace > start) end = lastSpace;
    }
    String seg = text.substring(start, end);
    seg.trim();
    lines[lineCount++] = seg;
    start = end;
  }

  int totalHeight = lineCount * lineHeight;
  int y = yCenter - totalHeight / 2 + lineHeight / 2;
  for (int i = 0; i < lineCount; i++) {
    lcd.drawString(lines[i], 120, y);
    y += lineHeight;
  }
}

void displayMessage(const String& line1, const String& line2, uint32_t color) {
  lcd.fillScreen(COL_BG);
  drawWrappedCentered(line1, 95, 22, color);
  if (line2.length() > 0) {
    drawWrappedCentered(line2, 155, 22, COL_TEXT);
  }
}
