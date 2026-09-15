#include <Arduino.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>
#include <vector>
#include "config.h"
#include "roster.h"
#include "flightstatus.h"
#include "display.h"

static Preferences prefs;
static String rosterUrl;
static String userName;
static String homeBase;

static std::vector<FlightEvent> flights;

static unsigned long lastRosterFetch = 0;
static unsigned long lastAdsbPoll = 0;
static unsigned long lastScreenSwitch = 0;
static int screenIndex = 0;

static time_t firstSeenAirborneUtc = 0;
static String activeFlightNumber;
static time_t activeFlightStart = 0;

struct LocationTzCache {
  String iata;
  float longitude = 0;
  bool valid = false;
};
static LocationTzCache tzCache;

static String utcTimeString(time_t nowUtc) {
  struct tm* t = gmtime(&nowUtc);
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
  return String(buf);
}

static String berlinTimeString(time_t utc, bool withDate) {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  struct tm* t = localtime(&utc);
  char buf[20];
  if (withDate) {
    snprintf(buf, sizeof(buf), "%02d.%02d. %02d:%02d", t->tm_mday, t->tm_mon + 1, t->tm_hour, t->tm_min);
  } else {
    snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
  }
  return String(buf);
}

static bool getLongitudeForIata(const String& iata, float& outLon) {
  if (tzCache.valid && tzCache.iata.equalsIgnoreCase(iata)) {
    outLon = tzCache.longitude;
    return true;
  }
  HTTPClient http;
  String url = "https://hexdb.io/api/v1/airport/iata/" + iata;
  if (!http.begin(url)) return false;
  int code = http.GET();
  if (code != HTTP_CODE_OK) { http.end(); return false; }
  String body = http.getString();
  http.end();

  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) return false;
  if (!doc.containsKey("longitude")) return false;

  outLon = doc["longitude"].as<float>();
  tzCache.iata = iata;
  tzCache.longitude = outLon;
  tzCache.valid = true;
  return true;
}

static String localTimeAtAirport(const String& iata, time_t nowUtc) {
  float lon;
  if (!getLongitudeForIata(iata, lon)) {
    return utcTimeString(nowUtc) + " UTC";
  }
  int offsetHours = (int)round(lon / 15.0);
  time_t local = nowUtc + (time_t)offsetHours * 3600;
  struct tm* t = gmtime(&local);
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
  return String(buf);
}

static void loadOrRequestConfig() {
  prefs.begin("flighttrk", false);
  rosterUrl = prefs.getString("rosterUrl", "");
  userName  = prefs.getString("userName", "");
  homeBase  = prefs.getString("homeBase", "");

  WiFiManager wm;
  WiFiManagerParameter rosterParam("roster", "Lufthansa Dienstplan-Link (ICS)", rosterUrl.c_str(), 300);
  WiFiManagerParameter nameParam("name", "Dein Vorname", userName.c_str(), 30);
  WiFiManagerParameter baseParam("base", "Heimatflughafen (IATA, z.B. FRA)", homeBase.c_str(), 4);
  wm.addParameter(&rosterParam);
  wm.addParameter(&nameParam);
  wm.addParameter(&baseParam);
  wm.setConfigPortalTimeout(180);

  displayWifiSetup();

  if (!wm.autoConnect(WIFI_AP_NAME)) {
    ESP.restart();
  }

  String enteredUrl = rosterParam.getValue();
  String enteredName = nameParam.getValue();
  String enteredBase = baseParam.getValue();
  enteredBase.toUpperCase();

  if (enteredUrl.length() > 0) { rosterUrl = enteredUrl; prefs.putString("rosterUrl", rosterUrl); }
  if (enteredName.length() > 0) { userName = enteredName; prefs.putString("userName", userName); }
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

static bool findFlightAfter(const FlightEvent& ref, FlightEvent& out) {
  bool found = false;
  time_t bestStart = 0;
  for (auto& e : flights) {
    if (e.startUtc > ref.startUtc && (!found || e.startUtc < bestStart)) {
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

      String line1 = userName + " ist nach " + f.arrIata + " gestartet";
      String line2 = "noch " + String(buf) + " Std bis Landung";
      displayMessage(line1, line2, COLOR_GREEN);
    } else {
      long elapsedMin = (long)difftime(now, f.startUtc) / 60;
      if (elapsedMin <= DELAYED_THRESHOLD_MIN) {
        String line1 = userName + "s Flug nach " + f.arrIata;
        String line2 = "ist puenktlich, startet gleich";
        displayMessage(line1, line2, COLOR_SKYBLUE);
      } else {
        String line1 = userName + "s Flug nach " + f.arrIata;
        String line2 = "ist verspaetet (+" + String(elapsedMin) + " Min)";
        displayMessage(line1, line2, COLOR_RED);
      }
    }
  }
}

static void showScreenNextFlight(time_t now) {
  FlightEvent next;
  if (!findNextFlight(now, next)) {
    displayMessage(userName + ": kein Flug", "geplant", COLOR_WHITE);
    return;
  }
  String line1 = "Naechster Flug: " + next.flightNumber;
  String line2 = next.depIata + "-" + next.arrIata + " " + berlinTimeString(next.startUtc, true) + " DE";
  displayMessage(line1, line2, COLOR_WHITE);
}

static void showScreenCurrentLocation(const String& currentLocation, time_t now) {
  String localT = localTimeAtAirport(currentLocation, now);
  String line1 = userName + " ist in " + currentLocation;

  FlightEvent next;
  String line2;
  if (findNextFlight(now, next)) {
    long mins = (long)difftime(next.startUtc, now) / 60;
    if (mins < 0) mins = 0;
    line2 = localT + " Uhr, Abflug in " + String(mins / 60) + "h" + String(mins % 60) + "m";
  } else {
    line2 = localT + " Uhr Ortszeit";
  }
  displayMessage(line1, line2, COLOR_SKYBLUE);
}

static void showScreenFlightAfterNext(time_t now) {
  FlightEvent next, afterNext;
  if (!findNextFlight(now, next) || !findFlightAfter(next, afterNext)) {
    displayMessage("Kein weiterer", "Flug bekannt", COLOR_WHITE);
    return;
  }
  String line1 = "Danach: " + afterNext.flightNumber;
  String line2 = afterNext.depIata + "-" + afterNext.arrIata + " " + berlinTimeString(afterNext.startUtc, true) + " DE";
  displayMessage(line1, line2, COLOR_WHITE);
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

  if (millis() - lastScreenSwitch > SCREEN_CYCLE_MS || lastScreenSwitch == 0) {
    screenIndex = (screenIndex + 1) % 3;
    lastScreenSwitch = millis();
  }

  switch (screenIndex) {
    case 0: showScreenNextFlight(now); break;
    case 1: showScreenCurrentLocation(currentLocation, now); break;
    case 2: showScreenFlightAfterNext(now); break;
  }

  delay(500);
}
