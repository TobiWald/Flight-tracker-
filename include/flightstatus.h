#pragma once
#include <Arduino.h>

struct LiveStatus {
  bool airborne = false;
  float lat = 0;
  float lon = 0;
  int altitudeFt = 0;
  int groundSpeedKt = 0;
};

// Fragt adsb.fi anhand der IATA-Flugnummer (z.B. "LH236") ab.
// Rechnet intern auf den ICAO-Callsign um (Lufthansa: LH -> DLH).
bool queryAdsbFiByFlightNumber(const String& flightNumber, LiveStatus& out);
