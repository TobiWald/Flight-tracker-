#pragma once
#include "roster.h"
#include "flightstatus.h"

void displayInit();
void displayWifiSetup();
void displayNoFlight();
void displayFlightScheduled(const FlightEvent& f, time_t now);
void displayFlightBoarding(const FlightEvent& f, int minutesToDeparture);
void displayFlightDelayed(const FlightEvent& f, int minutesLate);
void displayFlightAirborne(const FlightEvent& f, const LiveStatus& s, int delayMinutes);
void displayFlightLanded(const FlightEvent& f);
