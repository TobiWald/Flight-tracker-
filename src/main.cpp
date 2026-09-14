#include <Arduino.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <time.h>
#include <vector>
#include "config.h"
#include "roster.h"
#include "flightstatus.h"
#include "display.h"

static Preferences prefs;
static String rosterUrl;

static std::vector<FlightEvent> flights;
static FlightEvent activeFlight;
static bool hasActiveFlight = false;
static time_t firstSeenAirborneUtc = 0;

static unsigned long lastRosterFetch = 0;
static unsigned long lastAdsbPoll = 0;

static void loadOrRequestRosterUrl() {
  prefs.begin("flighttrk", false);
  rosterUrl = prefs.getString("rosterUrl", "");

  WiFiManager wm;
  WiFiManagerParameter rosterParam(
    "roster", "Lufthansa Dienstplan-Link (ICS)", rosterUrl.c_str(), 300);
  wm.addParameter(&rosterParam);
  wm.setConfigPortalTimeout(180);

  displayWifiSetup();

  // autoConnect zeigt das Setup-Portal nur, wenn keine gespeicherten WLAN-Zugangsdaten vorhanden sind
  if (!wm.autoConnect(WIFI_AP_NAME)) {
    ESP.restart();
  }

  String enteredUrl = rosterParam.getValue();
  if (enteredUrl.length() > 0) {
    rosterUrl = enteredUrl;
    prefs.putString("rosterUrl", rosterUrl);
  }
}

void setup() {
  Serial.begin(115200);
  displayInit();

  loadOrRequestRosterUrl();

  // Zeitzone Europe/Berlin inkl. automatischer Sommer-/Winterzeit-Umstellung
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  struct tm tmNow;
  while (!getLocalTime(&tmNow, 5000)) {
    delay(500);
  }

  fetchRoster(rosterUrl, flights);
  lastRosterFetch = millis();
}

void loop() {
  time_t now = time(nullptr);

  if (millis() - lastRosterFetch > ROSTER_REFRESH_INTERVAL_MS) {
    fetchRoster(rosterUrl, flights);
    lastRosterFetch = millis();
  }

  FlightEvent f;
  bool found = findActiveOrNextFlight(flights, now, f);

  if (!found) {
    displayNoFlight();
    delay(30000);
    return;
  }

  // Neuer Flug erkannt -> Delay-Tracking zurücksetzen
  if (!hasActiveFlight || f.flightNumber != activeFlight.flightNumber || f.startUtc != activeFlight.startUtc) {
    activeFlight = f;
    hasActiveFlight = true;
    firstSeenAirborneUtc = 0;
    lastAdsbPoll = 0;
  }

  double minsToStart = difftime(activeFlight.startUtc, now) / 60.0;
  double minsPastEnd = difftime(now, activeFlight.endUtc) / 60.0;

  if (minsPastEnd > LANDED_BUFFER_AFTER_MIN) {
    displayFlightLanded(activeFlight);
    delay(60000);
    return;
  }

  if (minsToStart > BOARDING_WINDOW_BEFORE_MIN) {
    displayFlightScheduled(activeFlight, now);
    delay(60000);
    return;
  }

  // Wir sind im aktiven Fenster (Boarding bis kurz nach geplanter Ankunft) -> Live-Status abfragen
  if (millis() - lastAdsbPoll > ADSB_POLL_INTERVAL_ACTIVE_MS || lastAdsbPoll == 0) {
    LiveStatus s;
    bool ok = queryAdsbFiByFlightNumber(activeFlight.flightNumber, s);
    lastAdsbPoll = millis();

    if (ok && s.airborne) {
      if (firstSeenAirborneUtc == 0) firstSeenAirborneUtc = now;
      int delayMin = (int)(difftime(firstSeenAirborneUtc, activeFlight.startUtc) / 60.0);
      if (delayMin < 0) delayMin = 0;
      displayFlightAirborne(activeFlight, s, delayMin);
    } else if (minsToStart <= 0 && (-minsToStart) > DELAYED_THRESHOLD_MIN) {
      displayFlightDelayed(activeFlight, (int)(-minsToStart));
    } else {
      displayFlightBoarding(activeFlight, (int)minsToStart);
    }
  }

  delay(5000);
}
