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
static String homeBase;

static std::vector<FlightEvent> flights;

static unsigned long lastRosterFetch = 0;
static unsigned long lastAdsbPoll = 0;
static unsigned long lastAwayMessageSwitch = 0;
static bool awayShowLocation = true;

static time_t firstSeenAirborneUtc = 0;
static String activeFlightNumber;
static time_t activeFlightStart = 0;

static String utcTimeString(time_t nowUtc) {
  struct tm* t = gmtime(&nowUtc);
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
  return String(buf);
}

static void loadOrRequestConfig() {
  prefs.begin("flighttrk", false);
  rosterUrl = prefs.getString("rosterUrl", "");
  homeBase  = prefs.getString("homeBase", "");

  WiFiManager wm;
  WiFiManagerParameter rosterParam("roster", "Lufthansa Dienstplan-Link (ICS)", rosterUrl.c_str(), 300);
  WiFiManagerParameter baseParam("base", "Heimatflughafen (IATA, z.B. FRA)", homeBase.c_str(), 4);
  wm.addParameter(&rosterParam);
  wm.addParameter(&baseParam);
  wm.setConfigPortalTimeout(180);

  displayWifiSetup();

  if (!wm.autoConnect(WIFI_AP_NAME)) {
    ESP.restart();
  }

  String enteredUrl = rosterParam.getValue();
  String enteredBase = baseParam.getValue();
  enteredBase.toUpperCase();

  if (enteredUrl.length() > 0) { rosterUrl = enteredUrl; prefs.putString("rosterUrl", rosterUrl); }
  if (enteredBase.length() > 0) { homeBase = enteredBase; prefs.putString("homeBase", homeBase); }
}

static bool findActiveFlight(time_t now, FlightEvent& out) {
  for (auto& e : flights) {
    if (now >= e.startUtc && now <= e.endUtc) { out = e; return true; }
  }
  return false;
}

static bool findLastCompletedFlight(time_t now, FlightEvent& out) {
  bool found = false;
  time_t bestEnd = 0;
  for (auto& e : flights) {
    if (e.endUtc <= now && (!found || e.endUtc > bestEnd)) {
      out = e; bestEnd = e.endUtc; found = true;
    }
  }
  return found;
}

static bool findNextFlight(time_t now, FlightEvent& out) {
  bool found = false;
  time_t bestStart = 0;
  for (auto& e : flights) {
    if (e.startUtc > now && (!found || e.startUtc < bestStart)) {
      out = e; bestStart = e.startUtc; found = true;
    }
  }
  return found;
}

static bool findNextReturnFlight(time_t now, FlightEvent& out) {
  bool found = false;
  time_t bestStart = 0;
  for (auto& e : flights) {
    if (e.startUtc > now && e.arrIata.equalsIgnoreCase(homeBase) && (!found || e.startUtc < bestStart)) {
      out = e; bestStart = e.startUtc; found = true;
    }
  }
  return found;
}

static void handleFlightStatus(const FlightEvent& f, time_t now) {
  if (activeFlightNumber != f.flightNumber || activeFlightStart != f.startUtc) {
    activeFlightNumber = f.flightNumber;
    activeFlightStart = f.startUtc;
    firstSeenAirborneUtc = 0;
    lastAdsbPoll = 0;
  }

  if (millis() - lastAdsbPoll > ADSB_POLL_INTERVAL_ACTIVE_MS || lastAdsbPoll == 0) {
    LiveStatus s;
    bool ok = queryAdsbFiByFlightNumber(f.flightNumber, s);
    lastAdsbPoll = millis();

    if (ok && s.airborne) {
      if (firstSeenAirborneUtc == 0) firstSeenAirborneUtc = now;
      long remainingSec = (long)difftime(f.endUtc, now);
      if (remainingSec < 0) remainingSec = 0;
      int hh = remainingSec / 3600;
      int mm = (remainingSec % 3600) / 60;
      char buf[8];
      snprintf(buf, sizeof(buf), "%02d:%02d", hh, mm);

      String line1 = String(USER_NAME) + " ist nach " + f.arrIata + " gestartet";
      String line2 = "noch " + String(buf) + " Std bis Landung";
      displayMessage(line1, line2, TFT_GREEN);
    } else {
      long elapsedMin = (long)difftime(now, f.startUtc) / 60;
      if (elapsedMin <= DELAYED_THRESHOLD_MIN) {
        String line1 = String(USER_NAME) + "s Flug nach " + f.arrIata;
        String line2 = "ist pünktlich, startet gleich";
        displayMessage(line1, line2, TFT_SKYBLUE);
      } else {
        String line1 = String(USER_NAME) + "s Flug nach " + f.arrIata;
        String line2 = "ist verspätet (+" + String(elapsedMin) + " Min)";
        displayMessage(line1, line2, TFT_RED);
      }
    }
  }
}

static void handleAwayState(const String& currentLocation, time_t now) {
  if (millis() - lastAwayMessageSwitch > AWAY_MESSAGE_SWITCH_MS || lastAwayMessageSwitch == 0) {
    awayShowLocation = !awayShowLocation;
    lastAwayMessageSwitch = millis();
  }

  if (awayShowLocation) {
    String line1 = String(USER_NAME) + " ist gerade in " + currentLocation;
    String line2 = "und hat " + utcTimeString(now) + " Uhr (UTC)";
    displayMessage(line1, line2, TFT_SKYBLUE);
  } else {
    FlightEvent ret;
    if (findNextReturnFlight(now, ret)) {
      long hours = (long)difftime(ret.startUtc, now) / 3600;
      if (hours < 0) hours = 0;
      String line1 = String(USER_NAME) + " fliegt in " + String(hours);
      String line2 = "Stunden zurück";
      displayMessage(line1, line2, TFT_SKYBLUE);
    } else {
      displayMessage("Rückflug noch", "nicht geplant", TFT_SKYBLUE);
    }
  }
}

static void handleHomeState(time_t now) {
  FlightEvent next;
  if (!findNextFlight(now, next)) {
    displayMessage(String(USER_NAME) + " ist zuhause", "kein anstehender Flug", TFT_WHITE);
    return;
  }
  long days = (long)difftime(next.startUtc, now) / 86400;
  String whenStr = (days <= 0) ? "heute" : ("in " + String(days) + " Tagen");
  String line1 = String(USER_NAME) + " muss " + whenStr;
  String line2 = "nach " + next.arrIata + " fliegen";
  displayMessage(line1, line2, TFT_WHITE);
}

void setup() {
  Serial.begin(115200);
  displayInit();

  loadOrRequestConfig();

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  struct tm tmNow;
  while (!getLocalTime(&tmNow, 5000)) { delay(500); }

  fetchRoster(rosterUrl, flights);
  lastRosterFetch = millis();
}

void loop() {
  time_t now = time(nullptr);

  if (millis() - lastRosterFetch > ROSTER_REFRESH_INTERVAL_MS) {
    fetchRoster(rosterUrl, flights);
    lastRosterFetch = millis();
  }

  FlightEvent active;
  if (findActiveFlight(now, active)) {
    handleFlightStatus(active, now);
    delay(2000);
    return;
  }

  FlightEvent lastCompleted;
  String currentLocation = homeBase;
  if (findLastCompletedFlight(now, lastCompleted)) {
    currentLocation = lastCompleted.arrIata;
  }

  if (currentLocation.equalsIgnoreCase(homeBase)) {
    handleHomeState(now);
  } else {
    handleAwayState(currentLocation, now);
  }

  delay(1000);
}
