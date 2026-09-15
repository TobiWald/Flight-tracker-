#pragma once

// Display-Pins (Waveshare ESP32-S3-Touch-LCD-1.47, JD9853, ST7789-kompatibel angesteuert)
#define LCD_PIN_DC   45
#define LCD_PIN_CS   21
#define LCD_PIN_SCK  38
#define LCD_PIN_MOSI 39
#define LCD_PIN_RST  47
#define LCD_WIDTH    172
#define LCD_HEIGHT   320
#define LCD_COL_OFFSET1 34
#define LCD_ROW_OFFSET1 0
#define LCD_COL_OFFSET2 34
#define LCD_ROW_OFFSET2 0

// Name des WLAN-Setup-Access-Points beim ersten Start
#define WIFI_AP_NAME "FlightTracker-Setup"

// Zeitintervalle
#define ROSTER_REFRESH_INTERVAL_MS   (15UL * 60UL * 1000UL)
#define ADSB_POLL_INTERVAL_ACTIVE_MS (60UL * 1000UL)
#define AWAY_MESSAGE_SWITCH_MS       (5UL * 1000UL)
#define DELAYED_THRESHOLD_MIN        15
#define LANDED_BUFFER_AFTER_MIN      20

#define ADSB_API_HOST "https://api.adsb.fi/v2/callsign/"
