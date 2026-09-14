#include "roster.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Parst ICS-Zeitformat "YYYYMMDDTHHMMSSZ" (immer UTC) in einen time_t-Wert
static time_t parseIcsUtc(const String& s) {
  if (s.length() < 15) return 0;
  struct tm t = {};
  t.tm_year = s.substring(0, 4).toInt() - 1900;
  t.tm_mon  = s.substring(4, 6).toInt() - 1;
  t.tm_mday = s.substring(6, 8).toInt();
  t.tm_hour = s.substring(9, 11).toInt();
  t.tm_min  = s.substring(11, 13).toInt();
  t.tm_sec  = s.substring(13, 15).toInt();
  return timegm(&t);
}

bool fetchRoster(const String& icsUrl, std::vector<FlightEvent>& outEvents) {
  WiFiClientSecure client;
  client.setInsecure(); // Lufthansa-Host nutzt eine öffentliche CA; vereinfacht das Zertifikats-Handling
  HTTPClient http;
  if (!http.begin(client, icsUrl)) return false;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  outEvents.clear();

  int pos = 0;
  String curStart, curEnd, curSummary, curLocation;
  bool inEvent = false;

  while (pos < (int)body.length()) {
    int nl = body.indexOf('\n', pos);
    if (nl < 0) nl = body.length();
    String line = body.substring(pos, nl);
    line.trim();
    pos = nl + 1;

    if (line == "BEGIN:VEVENT") {
      inEvent = true;
      curStart = curEnd = curSummary = curLocation = "";
      continue;
    }
    if (line == "END:VEVENT") {
      inEvent = false;
      // Echte Flugsegmente haben ein LOCATION-Feld mit " - " (z.B. "FRA - FCO").
      // Briefing/Layover/StandBy/Training haben nur einen einzelnen Ort.
      if (curLocation.indexOf(" - ") >= 0 && curSummary.length() > 0) {
        FlightEvent fe;
        fe.startUtc = parseIcsUtc(curStart);
        fe.endUtc = parseIcsUtc(curEnd);
        fe.route = curLocation;
        fe.isDeadhead = curSummary.startsWith("DH ");

        String s = curSummary;
        if (fe.isDeadhead) s = s.substring(3);
        int colon = s.indexOf(':');
        String designator = (colon >= 0) ? s.substring(0, colon) : s;
        designator.replace(" ", "");
        fe.flightNumber = designator; // z.B. "LH236"

        outEvents.push_back(fe);
      }
      continue;
    }
    if (!inEvent) continue;

    if (line.startsWith("DTSTART:")) curStart = line.substring(8);
    else if (line.startsWith("DTEND:")) curEnd = line.substring(6);
    else if (line.startsWith("SUMMARY:")) curSummary = line.substring(8);
    else if (line.startsWith("LOCATION:")) curLocation = line.substring(9);
  }

  return true;
}

bool findActiveOrNextFlight(const std::vector<FlightEvent>& events, time_t now, FlightEvent& out) {
  // Zuerst prüfen, ob gerade ein Flug aktiv ist
  for (auto& e : events) {
    if (now >= e.startUtc && now <= e.endUtc) {
      out = e;
      return true;
    }
  }
  // Sonst den zeitlich nächsten zukünftigen Flug suchen
  bool found = false;
  time_t bestStart = 0;
  for (auto& e : events) {
    if (e.startUtc > now && (!found || e.startUtc < bestStart)) {
      out = e;
      bestStart = e.startUtc;
      found = true;
    }
  }
  return found;
}
