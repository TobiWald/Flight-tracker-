#include "flightstatus.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Wandelt die IATA-Flugnummer aus dem Dienstplan in den ICAO-Callsign um, den
// Flugzeuge tatsächlich per ADS-B senden. Aktuell nur für Lufthansa-Mainline (LH -> DLH),
// da alle Flüge im Dienstplan mit "LH" beginnen.
static String toIcaoCallsign(const String& flightNumber) {
  if (flightNumber.startsWith("LH")) {
    return "DLH" + flightNumber.substring(2);
  }
  return flightNumber; // Fallback - wird dann vermutlich keinen Treffer liefern
}

bool queryAdsbFiByFlightNumber(const String& flightNumber, LiveStatus& out) {
  String callsign = toIcaoCallsign(flightNumber);
  String url = String(ADSB_API_HOST) + callsign;

  HTTPClient http;
  if (!http.begin(url)) return false;

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  StaticJsonDocument<4096> doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) return false;

  JsonArray ac = doc["ac"].as<JsonArray>();
  if (ac.isNull() || ac.size() == 0) {
    out.airborne = false;
    return true; // Antwort war gültig, es sendet aktuell nur kein Flugzeug unter diesem Callsign
  }

  JsonObject a = ac[0];
  out.airborne = true;
  out.lat = a["lat"] | 0.0f;
  out.lon = a["lon"] | 0.0f;
  out.altitudeFt = a["alt_baro"] | 0;
  out.groundSpeedKt = a["gs"] | 0;
  return true;
}
