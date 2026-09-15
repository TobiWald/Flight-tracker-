#pragma once
#include <Arduino.h>
#include <vector>
#include <time.h>

struct FlightEvent {
  time_t startUtc = 0;
  time_t endUtc = 0;
  String flightNumber;   // z.B. "LH236"
  String depIata;        // z.B. "FRA"
  String arrIata;        // z.B. "JFK"
  bool isDeadhead = false;
};

bool fetchRoster(const String& icsUrl, std::vector<FlightEvent>& outEvents);
