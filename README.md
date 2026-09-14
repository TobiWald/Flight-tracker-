# LH FlightTracker

Zeigt auf einem runden GC9A01-Display (ESP32-C3) den aktuellen Status deines
nächsten Lufthansa-Fluges: geplant, boarding, verspätet, in der Luft oder
gelandet - inklusive grober Verspätung in Minuten.

## Funktionsweise

1. Beim ersten Start öffnet der ESP32 ein WLAN-Setup-Portal ("FlightTracker-Setup").
   Dort trägst du dein Heim-WLAN UND deinen persönlichen Lufthansa-Dienstplan-Link
   (ICS-URL) ein. Der Link wird nur lokal im Flash gespeichert - niemals im Code.
2. Alle 15 Minuten wird der Dienstplan neu geladen und der nächste/aktive Flug bestimmt.
3. Ist ein Flug im aktiven Fenster (30 Min vor Abflug bis 20 Min nach geplanter
   Ankunft), wird minütlich bei adsb.fi per Callsign nachgefragt, ob das Flugzeug
   gerade sendet (= in der Luft ist).
4. Die "Verspätung" ist ein Näherungswert: Zeitpunkt, an dem das Flugzeug erstmals
   als airborne erkannt wird, verglichen mit der geplanten Abflugzeit aus dem
   Dienstplan. Keine offizielle Airline-Verspätungsangabe.

## Einschränkungen

- Funktioniert nur für Lufthansa-Mainline-Flüge (Callsign-Umrechnung LH -> DLH
  ist hart codiert).
- Wenn der Transponder aus irgendeinem Grund kein Signal liefert (z.B. am Gate
  vor dem Start), erscheint "Boarding" bzw. "Verspätet" statt "In der Luft",
  auch wenn technisch schon losgerollt wird.
- StandBy-Dienste ohne Flugnummer werden übersprungen; der Tracker zeigt dann
  automatisch den nächsten *echten* Flug danach.

## Setup

1. Repo bei GitHub anlegen (kann öffentlich bleiben, da der Dienstplan-Link
   NICHT im Code steht).
2. Eine Release erstellen (z.B. Tag `v1.0`) - das startet automatisch den
   Cloud-Build über GitHub Actions.
3. Die gebaute `FlightTracker.bin` aus dem Release-Anhang herunterladen und
   wie gewohnt per Web-Serial-Tool (z.B. webinstaller.esp3d.io) flashen.
4. Beim ersten Boot mit dem AP "FlightTracker-Setup" verbinden, WLAN + den
   Lufthansa-ICS-Link eingeben.
