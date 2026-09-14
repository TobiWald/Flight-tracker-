#include "display.h"
#include "lgfx_config.hpp"

static LGFX lcd;

// Farben
#define COL_BG       TFT_BLACK
#define COL_SCHEDULE TFT_SKYBLUE
#define COL_BOARDING TFT_ORANGE
#define COL_DELAYED  TFT_RED
#define COL_AIRBORNE TFT_GREEN
#define COL_LANDED   TFT_LIGHTGREY
#define COL_TEXT     TFT_WHITE

static void drawBase(uint16_t statusColor, const char* statusText) {
  lcd.fillScreen(COL_BG);
  lcd.setTextDatum(top_center);
  lcd.setTextColor(statusColor);
  lcd.setTextSize(2);
  lcd.drawString(statusText, 120, 20);
}

static String formatHHMM(time_t utc) {
  struct tm* t = localtime(&utc); // TZ wird in main.cpp gesetzt
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
  return String(buf);
}

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

void displayNoFlight() {
  drawBase(COL_LANDED, "KEIN FLUG");
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(COL_TEXT);
  lcd.setTextSize(2);
  lcd.drawString("Kein anstehender", 120, 110);
  lcd.drawString("Flug im Dienstplan", 120, 135);
}

void displayFlightScheduled(const FlightEvent& f, time_t now) {
  drawBase(COL_SCHEDULE, "GEPLANT");
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(COL_TEXT);
  lcd.setTextSize(3);
  lcd.drawString(f.flightNumber, 120, 90);
  lcd.setTextSize(2);
  lcd.drawString(f.route, 120, 125);
  lcd.drawString("Abflug " + formatHHMM(f.startUtc), 120, 155);
}

void displayFlightBoarding(const FlightEvent& f, int minutesToDeparture) {
  drawBase(COL_BOARDING, "BOARDING");
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(COL_TEXT);
  lcd.setTextSize(3);
  lcd.drawString(f.flightNumber, 120, 90);
  lcd.setTextSize(2);
  lcd.drawString(f.route, 120, 125);
  String rel = (minutesToDeparture >= 0)
    ? ("noch ca. " + String(minutesToDeparture) + " Min")
    : "gleich";
  lcd.drawString(rel, 120, 155);
}

void displayFlightDelayed(const FlightEvent& f, int minutesLate) {
  drawBase(COL_DELAYED, "VERSPAETET");
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(COL_TEXT);
  lcd.setTextSize(3);
  lcd.drawString(f.flightNumber, 120, 90);
  lcd.setTextSize(2);
  lcd.drawString(f.route, 120, 125);
  lcd.drawString("+" + String(minutesLate) + " Min", 120, 155);
}

void displayFlightAirborne(const FlightEvent& f, const LiveStatus& s, int delayMinutes) {
  drawBase(COL_AIRBORNE, "IN DER LUFT");
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(COL_TEXT);
  lcd.setTextSize(3);
  lcd.drawString(f.flightNumber, 120, 80);
  lcd.setTextSize(2);
  lcd.drawString(f.route, 120, 112);
  String delayText = (delayMinutes > 0) ? ("+" + String(delayMinutes) + " Min") : "pünktlich";
  lcd.drawString(delayText, 120, 140);
  lcd.setTextSize(1);
  lcd.drawString(String(s.altitudeFt) + " ft  " + String(s.groundSpeedKt) + " kt", 120, 168);
}

void displayFlightLanded(const FlightEvent& f) {
  drawBase(COL_LANDED, "GELANDET");
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(COL_TEXT);
  lcd.setTextSize(3);
  lcd.drawString(f.flightNumber, 120, 100);
  lcd.setTextSize(2);
  lcd.drawString(f.route, 120, 135);
}
