#pragma once

// Display-Pins (ESP32-2424S012C / GC9A01 240x240 rund) - schon bekannt vom Plane-Radar-Projekt
#define DISPLAY_PIN_RST   GPIO_NUM_NC
#define DISPLAY_PIN_CS    GPIO_NUM_10
#define DISPLAY_PIN_DC    GPIO_NUM_2
#define DISPLAY_PIN_MOSI  GPIO_NUM_7
#define DISPLAY_PIN_SCLK  GPIO_NUM_6
#define DISPLAY_PIN_BL    GPIO_NUM_3
#define DISPLAY_BL_PWM_CHANNEL 0

// Name des WLAN-Setup-Access-Points beim ersten Start
#define WIFI_AP_NAME "FlightTracker-Setup"

// Zeitintervalle
#define ROSTER_REFRESH_INTERVAL_MS   (15UL * 60UL * 1000UL)   // Dienstplan alle 15 Min neu laden
#define ADSB_POLL_INTERVAL_ACTIVE_MS (60UL * 1000UL)          // Live-Status jede Minute im aktiven Fenster
#define BOARDING_WINDOW_BEFORE_MIN   30   // ab wann vor Abflug "Boarding" angezeigt wird
#define DELAYED_THRESHOLD_MIN        15   // ab wann "verspätet" statt "boarding" angezeigt wird
#define LANDED_BUFFER_AFTER_MIN      20   // Minuten nach geplanter Ankunft, ab der "gelandet" angenommen wird

#define ADSB_API_HOST "https://api.adsb.fi/v2/callsign/"
