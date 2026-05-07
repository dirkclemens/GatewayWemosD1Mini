# GatewayWemosD1Mini

MySensors WiFi Gateway auf Basis des **Wemos D1 Mini (ESP8266)** mit moderner Web-UI, Telnet-Interface und Pushover-Benachrichtigungen.

Version: **2.4.1**

## Features

- 📡 **MySensors WiFi-Gateway** — empfängt nRF24L01+-Funknachrichten und leitet sie ins Netzwerk weiter
- 🌐 **Moderne Single-Page Web-UI** (5 Tabs, Server-Sent Events, kein Polling)
- 📟 **Telnet-Interface** für Live-Debugging
- 🔔 **Pushover-Benachrichtigungen** (optional)
- 🕐 **NTP-Zeitsynchronisation**
- ⚡ **OTA-Updates** über PlatformIO oder `espota.py`
- 💾 HTML/CSS/JS vollständig in PROGMEM (Flash) — kein RAM-Verbrauch für die UI

## Web-UI

Die Web-Oberfläche ist über `http://<gateway-ip>/` erreichbar und enthält fünf Tabs:

| Tab | Inhalt |
|-----|--------|
| **📨 Messages** | Live-Tabelle der letzten 30 MySensors-Nachrichten (Zeit, Node, Sensor, Cmd, Typ, Payload) |
| **📊 Sensors** | Pro-Node-Karten mit dem letzten empfangenen Wert (`C_SET`) je Sensor inkl. Zeitstempel — Daten leben im Browser-Cache, kein ESP-RAM |
| **ℹ️ Info** | Gateway-Statistiken (Heap, RSSI, Rx/Tx-Zähler) + Aktionen: Reboot, Reconnect, Bootlog, Log löschen |
| **🏷️ Nodes** | Node-Namen vergeben und verwalten (persistent auf dem Gateway gespeichert) |
| **🔧 Debug** | Telemetrie-Badges, Heap/Frag-Chart, Log-Level-Filter, Debug- und Indicator-Log, LED-Status |

### Screenshots

#### 📨 Messages
![Messages Tab](screenshots/messages.png)

#### 📊 Sensors
![Sensors Tab](screenshots/sensors.png)

#### ℹ️ Info
![Info Tab](screenshots/info.png)

#### 🏷️ Nodes
![Nodes Tab](screenshots/nodes.png)

#### 🔧 Debug
![Debug Tab](screenshots/debug.png)

### Sensor-Tab

Alle eingehenden `C_SET`-Nachrichten werden im Browser-Speicher der aktuellen Session gehalten. Beim Wechsel zwischen Tabs bleiben die Daten erhalten. Es werden keine Messwerte auf dem ESP8266 gespeichert.

## Konfiguration

Datei `credentials.h` im `src/`-Verzeichnis erstellen:

```cpp
#define MY_WIFI_SSID     "ssid"
#define MY_WIFI_PASSWORD "passwort"
const char *_token = "...";   // Pushover API-Token
const char *_user  = "...";   // Pushover User-Key
```

Feature-Flags in `src/config.h`:

```cpp
#define OTA        // OTA-Updates aktivieren
#define WWW        // Web-UI aktivieren (Port 80)
#define TELNET     // Telnet-Interface (Port 23)
#define NTP        // Zeitsynchronisation
#define PUSHOVER   // Pushover-Benachrichtigungen
```

## Build & Flash

**PlatformIO** (empfohlen):

```bash
pio run                          # kompilieren
pio run -t upload                # seriell flashen
pio run -e d1_mini_ota -t upload # OTA flashen
```

**OTA manuell:**

```bash
python espota.py -d -i 192.168.x.x -f .pio/build/d1_mini_ota/firmware.bin
```

**Seriell (esptool):**

```bash
ls /dev/cu.*
./esptool -vv -cd nodemcu -cb 115200 -cp /dev/cu.usbserial-XXXX -ca 0x00000 -cf firmware.bin
```

### PlatformIO-Umgebungen

| Umgebung | Beschreibung |
|----------|-------------|
| `d1_mini_ota` | Wemos D1 Mini, OTA-Upload (Standard) |
| `d1_mini_serial` | Wemos D1 Mini, serieller Upload |
| `d1_mini_pro_serial` | Wemos D1 Mini Pro, serieller Upload |

## Monitoring / Crash-Erkennung

### `/healthz` Endpoint

Der Gateway stellt einen schlanken JSON-Endpoint bereit:

```text
GET /healthz
```

Beispiel-Felder: `boot_count`, `last_boot_epoch`, `reset_reason_code`, `uptime_s`, `heap`, `rssi`, `wifi_connected`, `sse_clients`.

### Dateizugriff auf LittleFS

- `GET /fs` → listet Dateien als JSON
- `GET /fs?path=/bootlog.txt` → liefert die angeforderte Datei

Pfadvalidierung ist aktiv (`/`-Pflicht, kein `..`).

### Externer Heartbeat (optional)

In `src/config.h` kann ein Outbound-Heartbeat aktiviert werden:

```cpp
// #define EXTERNAL_HEARTBEAT_URL "http://192.168.2.222:8080/mysensors/heartbeat"
#define EXTERNAL_HEARTBEAT_INTERVAL_MS 60000UL
```

Wenn `EXTERNAL_HEARTBEAT_URL` gesetzt ist, sendet der Gateway regelmäßig einen kleinen HTTP-POST mit Boot-/Heap-/WiFi-Metadaten.

#### Minimaler Python-Receiver (Flask)

Datei: `tools/heartbeat_server.py`

Start auf dem Smarthome-Server:

```bash
python3 -m pip install flask
HB_BIND_HOST=192.168.2.222 HB_BIND_PORT=18080 python3 tools/heartbeat_server.py
```

Verfügbare Endpoints:

- `POST /mysensors/heartbeat` (vom ESP)
- `GET /mysensors/heartbeat/latest?host=GatewayWemosD1Mini`
- `GET /mysensors/heartbeat/history?limit=50`
- `GET /healthz`

#### Als systemd-Service

Service-Template: `tools/mysensors-heartbeat.service`  
Env-Template: `tools/mysensors-heartbeat.env.example`

```bash
sudo cp tools/mysensors-heartbeat.service /etc/systemd/system/
sudo cp tools/mysensors-heartbeat.env.example /etc/default/mysensors-heartbeat
```

Dann in `/etc/systemd/system/mysensors-heartbeat.service` den Projektpfad (`/opt/GatewayWemosD1Mini`) anpassen.

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now mysensors-heartbeat
sudo systemctl status mysensors-heartbeat
```

### Monit: Reboot-Erkennung (nicht nur Down)

Zusätzlich zum HTTP-Alive-Check kann Monit den `boot_count` überwachen:

```monit
CHECK PROGRAM mysensorsgw_bootcount WITH PATH "/usr/local/bin/check-mysensorsgw-bootcount.sh"
  IF STATUS != 0 FOR 2 CYCLES THEN EXEC "/etc/monit/alert-ntfy.sh 'mysensorsGW reboot' 'boot_count geändert'"
```

Beispiel `check-mysensorsgw-bootcount.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail
STATE="/var/lib/monit/mysensorsgw.bootcount"
URL="http://192.168.2.211/healthz"

boot_count="$(curl -fsS "$URL" | sed -n 's/.*"boot_count":\([0-9]\+\).*/\1/p')"
[[ -n "$boot_count" ]]

if [[ -f "$STATE" ]]; then
  old="$(cat "$STATE")"
  echo "$boot_count" > "$STATE"
  [[ "$boot_count" == "$old" ]] && exit 0 || exit 1
fi

echo "$boot_count" > "$STATE"
exit 0
```

## Hardware

### LED-Belegung

| LED | Farbe | Bedeutung |
|-----|-------|-----------|
| RX | grün | Blinkt bei empfangener Funknachricht |
| TX | gelb | Blinkt bei gesendeter Funknachricht |
| ERR | rot | Blinkt schnell bei Übertragungsfehler/CRC-Fehler |

### nRF24L01+ Verkabelung

| nRF24L01+ | ESP8266 | Wemos D1 mini | Hinweis |
|-----------|---------|---------------|---------|
| VCC | VCC | VCC | |
| CE | GPIO4 | D2 | |
| CSN/CS | GPIO15 | D8 | 10K Pulldown nach GND |
| SCK | GPIO14 | D5 | |
| MISO | GPIO12 | D6 | |
| MOSI | GPIO13 | D7 | |
| GND | GND | GND | |
| — | CH_PD | — | 10K Pullup nach VCC |
| — | GPIO2 | D4 | 10K Pullup nach VCC |
| — | GPIO0 | D3 | 10K Pullup nach VCC + Taster nach GND (Bootload) |
| — | GPIO16 | D0 | frei |
| — | GPIO5 | D1 | Taster nach GND (Inclusion Mode) |

Für Bare-ESP-Module (z. B. ESP-12E): RST via 10K Pullup nach VCC + Taster nach GND.

```cpp
#define MY_DEFAULT_ERR_LED_PIN D10  // Error LED (Rot)
#define MY_DEFAULT_RX_LED_PIN  D9   // Receive LED (Gelb)
#define MY_DEFAULT_TX_LED_PIN  D1   // Transmit LED (Grün)
```

## Ressourcen

- [MySensors ESP8266 Gateway](https://www.mysensors.org/build/esp8266_gateway)
- [MySensors Advanced Gateway](https://www.mysensors.org/build/advanced_gateway)
- [Wemos D1 Mini](https://www.wemos.cc/product/d1-mini.html)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
