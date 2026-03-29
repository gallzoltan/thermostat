# Termosztát v01

ESP8266 alapú WiFi-s termosztát DS18B20 hőmérséklet szenzorral, SSD1306 OLED kijelzővel és rotary encoder vezérléssel. Az aktuális állapotot HTTP-n keresztül szinkronizálja egy helyi szerverrel.

## Hardware

| Komponens | Típus |
|-----------|-------|
| Mikrokontroller | ESP8266 (NodeMCU / Wemos D1) |
| Hőmérséklet szenzor | DS18B20 (OneWire) |
| Kijelző | SSD1306 OLED 128x64 (I2C) |
| Vezérlő | Rotary encoder nyomógombbal |
| Kimenet | Relé (fűtésvezérlés) |

### Pin kiosztás

| Pin | GPIO | Funkció |
|-----|------|---------|
| ONE_WIRE_BUS | 0 | DS18B20 szenzor |
| PIN_CLK | 14 | Encoder órajel |
| PIN_DT | 12 | Encoder adat |
| PIN_SW | 13 | Encoder gomb |
| PIN_RELAY | 15 | Relé |
| PIN_SCL | 4 | I2C SCL (kijelző) |
| PIN_SDA | 5 | I2C SDA (kijelző) |

## Működés

- **Auto mód**: a célhőmérsékletet (`setTempAuto`) a szerver adja 60 másodpercenként
- **Manuális mód**: rotary encoderrel állítható a célhőmérséklet (0.5°C lépésekben, 10–32°C között)
- Encoder **gombnyomás** visszakapcsol auto módba
- A relay hisztézissel kapcsol: BE ha `currentTemp < setTemp - 0.2°C`, KI ha `currentTemp > setTemp + 0.2°C`
- Az állapot (hőmérséklet, beállítás, relay, mód) 30 másodpercenként PUT kéréssel kerül a szerverre

## Szerver API

Alapértelmezett cím: `http://thermo` (helyi hálózaton)

| Végpont | Metódus | Leírás |
|---------|---------|--------|
| `/insert` | PUT | Aktuális állapot küldése JSON-ban |
| `/get` | GET | Célhőmérséklet lekérése (`setterm` mező) |

PUT payload példa:
```json
{
  "currTemp": "22.50",
  "adjustTemp": "21.00",
  "circoState": "1",
  "controlMode": "0"
}
```

## Szükséges könyvtárak

- ESP8266WiFi
- ESP8266HTTPClient
- ArduinoJson (v6+)
- SSD1306 (ESP8266 OLED Driver)
- OneWire
- DallasTemperature
- Ticker

## Build és feltöltés

**Arduino IDE:** Nyisd meg a `thermostat_01.ino` fájlt, válaszd ki az ESP8266 boardot, majd fordítsd és töltsd fel.

**arduino-cli:**
```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 thermostat_01.ino
arduino-cli upload  --fqbn esp8266:esp8266:nodemcuv2 -p /dev/ttyUSB0 thermostat_01.ino
```

## Konfiguráció

A WiFi hitelesítő adatok a `thermostat_01.ino` fájl elején állíthatók:
```cpp
const char* ssid = "network_name";
const char* password = "network_password";
const String server = "http://thermo";
```
