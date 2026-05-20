# Projektdokumentation: GatewayWemosD1Mini2026

## 1. Projektüberblick

`GatewayWemosD1Mini2026` ist ein MySensors-WLAN-Gateway für ESP8266 (Wemos D1 mini/D1 mini Pro) und optional ESP32, das RF24-Funksensordaten ins IP-Netz überführt und eine lokale Betriebs-/Diagnoseoberfläche bereitstellt.

Hauptfunktionen:
- MySensors Gateway (ESP8266/ESP32, RF24)
- Web-Oberfläche mit Server-Sent Events (Live-Updates)
- Telnet-Ausgabe für Live-Logs
- OTA-Update-Unterstützung
- NTP-Zeitsynchronisierung
- Pushover-Benachrichtigung bei kritischem Heap
- Persistentes Bootlog im LittleFS
- ARC-Statistik (Automatic Retries Count) für Funkqualität

## 2. Hardware- und Software-Basis

- MCU:
  - ESP8266 (80 MHz)
  - ESP32 (bei aktivem Build-Flag `USE_ESP32`)
- Zielboards:
  - `d1_mini` (4 MB Flash)
  - `d1_mini_pro` (16 MB Flash)
  - `esp32dev` (Environment `esp32_devkitc_serial` / `esp32_devkitc_ota`)
- Funk: nRF24L01+ über `MY_RADIO_RF24`
- Framework: Arduino
- Buildsystem: PlatformIO

Wichtige Plattformbindungen:
- ESP8266: `espressif8266@2.6.2` (kompatibilitätskritisch)
- ESP32: `espressif32@6.10.0`

## 3. Build- und Deployment-Konfiguration

Datei: `platformio.ini`

### 3.1 Umgebungen
- `d1_mini_ota` (Default)
- `d1_mini_serial`
- `d1_mini_pro_serial`
- `esp32_devkitc_serial`
- `esp32_devkitc_ota`

### 3.2 Relevante Einstellungen
- Dateisystem: `LittleFS`
- Linkerscript (4 MB): `eagle.flash.4m1m.ld`
- Bibliotheken:
  - `MySensors@>=2.3.2`
  - `ESPAsyncTCP`
  - `ESP Async WebServer`
- Monitor-Baudrate:
  - OTA-Env: `115200`
  - Serial-Env: `9600` (passend zu `MY_BAUD_RATE 9600`)

### 3.3 OTA
- OTA ist per Makro aktiv (`OTA` in `src/config.h`)
- Beispielskript: `otaupload.sh`

## 4. Feature-Toggles und zentrale Konfiguration

Datei: `src/config.h`

Aktivierte Funktionen (Compile-Time):
- `OTA`
- `WWW`
- `TELNET`
- `NTP`
- `PUSHOVER`

Weitere zentrale Konstanten:
- Softwareversion: `__SWVERSION__ = "2.4.1"`
- Web-Port: `80`
- Telnet-Port: `23`
- Zeitzone: CET/CEST via `TZ_INFO`
- NTP-Serverliste in `NTP_SERVER[]`

## 5. Zugangsdaten / Secrets

Das Projekt erwartet außerhalb des Repo-Baums eine Datei:
- `../../credentials.h` (relativ zu `src/main.cpp`)

Benötigte Inhalte laut `readme.md`:
- `MY_WIFI_SSID`
- `MY_WIFI_PASSWORD`
- `_token` (Pushover)
- `_user` (Pushover)

Hinweis: Secrets sind korrekt aus dem Repository ausgelagert.

## 6. Laufzeitarchitektur

### 6.1 Hauptmodule
- `src/main.cpp`
  - Gateway-Konfiguration (MySensors-Makros)
  - Setup/Loop-Lebenszyklus
  - Verarbeitung eingehender MySensors-Nachrichten (`receive`)
  - Statistik/ARC-Erfassung
  - OTA-/NTP-/WiFi-/Telnet-Loopteile
- `src/WebServer.cpp`
  - Async-Webserver
  - EventSource `/events`
  - HTTP-Endpunkte (`/`, `/stats`, `/bootlog.txt`, etc.)
- `src/common.cpp/.h`
  - Debug-Logging-Helfer
  - Zeitformatierung
  - Speichergrößenformatierung
- `src/uptime.cpp/.h`
  - Runtime/Uptime-Funktionen
  - Bootzeitstempel
- `src/mysensors_types.h`
  - Tabellen für MySensors Command/Type/Unit-Namen

### 6.2 Ablauf in `setup()`
1. Reset-Information erfassen
2. Optional WLAN-Setup (nur bei `MY_CORE_ONLY`)
3. Telnet-Server starten
4. NTP initialisieren (`configTime`)
5. Webserver initialisieren
6. OTA initialisieren
7. LittleFS mounten
8. Boot-/Restart-Metadaten laden/fortschreiben (`/boot_count.txt`, `/last_boot_epoch.txt`, `/last_restart_marker.txt`)
9. Statistiken initialisieren
10. Bootzeit setzen
11. Bootlog in `/bootlog.txt` fortschreiben

### 6.3 Ablauf in `loop()`
- CPU-Zykluszeit erfassen
- Watchdog-freundliche `delay/yield`
- Tageswechsel erkennen und Tageszähler zurücksetzen
- WLAN-Reconnect, falls getrennt (zweistufige Recovery)
- Webserver-/Telnet-/NTP-/OTA-Loopteile bedienen
- Jede Sekunde:
  - Heap/Fragmentierung und Timingdaten erzeugen
  - Live-Debug/Event aktualisieren
  - Statusinfo für Web-UI aktualisieren
  - Optional Pushover bei Heap < 10 KB
- Alle 15 Minuten:
  - Heartbeat senden
  - Uptime senden
  - ARC-Statistik senden

WLAN-Recovery-Strategie:
- Stage 1: nach 5 Minuten ohne WLAN wird der WiFi-Stack hart neu initialisiert.
- Stage 2: bleibt WLAN danach weiterhin aus, wird nach dem Timeout-Fenster rebootet.
- Geplante Neustarts werden als Marker persistiert und beim nächsten Boot ausgewertet.

## 7. MySensors-Integration

### 7.1 Gateway-Modus
- Standard: `MY_GATEWAY_ESP8266`
- Optional (kommentiert): MQTT-Gateway (`WITH_MQTT`)

### 7.2 Netzwerk
- Gateway-Port (IP-Gateway): `5003`
- Max. Clients: `MY_GATEWAY_MAX_CLIENTS 4`

### 7.3 Inclusion/LED
- Inclusion-Feature aktiv
- Inclusion-Button-Feature aktiv
- Dauer: 60 Sekunden
- LED-Blinking konfiguriert (`MY_DEFAULT_ERR_LED_PIN`, `MY_DEFAULT_RX_LED_PIN`, `MY_DEFAULT_TX_LED_PIN`)

### 7.4 Präsentierte Childs
- `CHILD_ID_UPTIME` (`S_INFO`)
- `SENSOR_ID_ARC=98` als `S_CUSTOM` für ARC-JSON

### 7.5 Empfangsverarbeitung
In `receive(const MyMessage&)` werden Nachrichten:
- nach Command-Typ dekodiert (`C_PRESENTATION`, `C_SET`, `C_INTERNAL`, ...)
- Payload typgerecht formatiert
- auf Telnet ausgegeben
- als JSON-artiges Event `messagesjson` an die Web-UI gesendet

## 8. Weboberfläche und HTTP-Endpunkte

Implementierung: `src/WebServer.cpp`, UI-Assets in `src/index.h`, `src/style_css.h`, `src/svg_gz.h`.

### 8.1 Transport
- Server-Sent Events über `GET /events`
- Event-Typen:
  - `debug`
  - `info`
  - `indicator`
  - `led`
  - `messagesjson`

UI-Struktur (5 Tabs):
- Messages
- Sensors
- Settings (inkl. Node-Namen und Laufzeit-Settings)
- Info
- Debug

### 8.2 Endpunkte
- `GET /`:
  - Hauptseite (Live-Messages + Info-Tab)
- `GET /style.css`:
  - Styles aus PROGMEM
- `GET /favicon.ico`, `GET /mask-icon.svg`:
  - komprimierte Assets aus PROGMEM
- `GET /stats`:
  - Textstatus mit System-, WLAN-, Flash-, Heap-, FS- und Bootlog-Informationen
- `GET /healthz`:
  - JSON-Status inkl. `boot_count`, `reset_reason`, `reset_reason_raw`, `planned_restart_marker`, Heap, WiFi, RSSI
- `GET /bootlog.txt`:
  - Inhalt des Bootlogs aus LittleFS
- `GET /wipe`:
  - löscht `/bootlog.txt`
- `GET /reconnect`:
  - WLAN-Reconnect
- `POST /reboot`:
  - setzt Reboot-Flag, tatsächlicher Reboot erfolgt im Loop
- `GET /api/web-settings`, `POST /api/web-settings`:
  - Laufzeit-Settings (Debug, Live-Updates, Telnet, OTA, UI-Intervall)
- `GET /api/node-names`, `POST /api/node-name`:
  - Node-Namen lesen/schreiben
- `GET /api/sensor-state`:
  - Letzte Sensorwerte für den Sensor-Tab

### 8.3 Frontend-Verhalten
- EventSource empfängt Live-Daten
- `messagesjson` wird in eine tabellarische Ansicht umgewandelt
- Ringpuffer (JavaScript CircularQueue) für die letzten 30 Meldungen
- Die Info-Ansicht ist konsolidiert: MQTT-/Client-Infos werden direkt in den Info-Block integriert (kein separater Clients-Bereich)

## 9. Dateisystem (LittleFS)

Verwendung:
- Persistentes Bootlog: `/bootlog.txt`
- Bootzähler: `/boot_count.txt`
- Letzter Bootzeitpunkt: `/last_boot_epoch.txt`
- Persistenter Restart-Marker: `/last_restart_marker.txt`
- Diagnoseausgabe via `/stats`
- Lesen/Löschen via Web-Endpunkte

Wichtige Nebenwirkung:
- OTA-Start ruft `LittleFS.end()` auf (sauber vor Updatevorgang)

## 10. Benachrichtigungen (Pushover)

- HTTPS-POST an `https://api.pushover.net/1/messages.json`
- TLS-Client mit `setInsecure()`
- Trigger im Sekundentakt bei niedrigem Heap (`<10 KB`, mit Anti-Spam-Flag)

## 11. Logging und Diagnose

### 11.1 Serielle/Telnet-Ausgabe
- `telnetOut` spiegelt optional auf Serial + Telnet
- Umfangreiche Debugfunktionen in `common.cpp` (mit Icon-Präfixen)

### 11.2 Laufzeit-/Speicherdaten
Regelmäßig publiziert:
- Runtime/Uptime
- Heap-Größe, Fragmentierung, größter freier Block
- Zykluszeit (`micros`-basierte Delta-Messung)

### 11.3 ARC-Statistik
- Ermittlung über `transportHALGetSendingRSSI()`-basierte Retry-Schätzung
- Kennzahlen:
  - `P` Pakete
  - `R` Retries
  - `S` Erfolgsrate in %
- Versand als JSON-String, z. B. `{P:100,R:10,S:90}`

## 12. Verzeichnisstruktur (relevant)

- `src/main.cpp`: Hauptlogik
- `src/WebServer.cpp/.h`: Async-Webserver + SSE
- `src/common.cpp/.h`: Utils/Debug
- `src/uptime.cpp/.h`: Uptime/Boottime
- `src/config.h`: Feature-Makros, Ports, Version, NTP
- `src/mysensors_types.h`: Mappingtabellen für MySensors
- `src/index.h`: HTML + JS (PROGMEM)
- `src/style_css.h`: CSS (PROGMEM)
- `src/svg_gz.h`: komprimierte SVG-Assets
- `platformio.ini`: Build- und Uploadprofile
- `readme.md`: Kurzanleitung/Wiring
- `otaupload.sh`: OTA-Helferskript
- `data/`, `static/`: Asset-Quellen/Varianten für Webinhalte

## 13. Betriebshinweise

- Für stabile Kompatibilität mit diesem Code ist die fixierte ESP8266-Plattformversion wichtig (`2.6.2`).
- Das Projekt ist stark compile-time-gesteuert (Makros). Änderungen an Features erfolgen primär über `src/config.h` und MySensors-Makros in `src/main.cpp`.
- Die Weboberfläche ist vollständig eingebettet (PROGMEM) und benötigt keine externen Webdateien zur Laufzeit.

## 14. Bekannte technische Auffälligkeiten

- Das Projekt unterstützt zwei Zielplattformen (ESP8266/ESP32). Nicht jedes Feature ist auf beiden Plattformen identisch tief instrumentiert.
- Bei sehr großer Info-/Debug-Ausgabe kann die UI-Last steigen; daher sind Live-Updates und Debug einzeln schaltbar.
- Einige Texte sind gemischt Deutsch/Englisch; für Teamwartung wäre einheitliche Sprache sinnvoll.

## 15. Empfohlene nächste Schritte

1. `credentials.h`-Pfad vereinheitlichen (z. B. über Build-Flags/Include-Pfad), um Portabilität zu erhöhen.
2. Optional: Web-UI und Asset-Quellen (`data/static` vs. PROGMEM-Headers) mit einem klaren Build-Generierungsprozess dokumentieren.
3. Optional: Kleine Tests/Checks für ARC-Reporting, Bootlog-Rotation und Heap-Alarm ergänzen.
4. Optional: Crash-Backtrace-Dekodierung als festen Wartungsablauf (inkl. Script) dokumentieren.

