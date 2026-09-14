#pragma once
#include <Arduino.h>
#include <vector>
#include <time.h>

struct FlightEvent {
  time_t startUtc = 0;
  time_t endUtc = 0;
  String flightNumber;   // z.B. "LH236"
  String route;          // z.B. "FRA - FCO"
  bool isDeadhead = false;
};

// Lädt die ICS-Datei von der Roster-URL und extrahiert nur echte Flugsegmente
// (Briefing/Layover/StandBy/Training werden anhand des LOCATION-Feldes herausgefiltert)
bool fetchRoster(const String& icsUrl, std::vector<FlightEvent>& outEvents);

// Findet den aktuell aktiven Flug (jetzt zwischen Start/Ende) oder sonst den nächsten anstehenden
bool findActiveOrNextFlight(const std::vector<FlightEvent>& events, time_t now, FlightEvent& out);
