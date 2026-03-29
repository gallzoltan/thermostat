# CLAUDE.md

Ez a fájl útmutatást nyújt a Claude Code (claude.ai/code) számára, amikor ezzel a repository-val dolgozik.

## Projekt áttekintés

Arduino termosztát **ESP8266** mikrokontrolleren. DS18B20 szenzorból olvassa a hőmérsékletet, relén keresztül vezérli a fűtést, SSD1306 OLED kijelzőn jeleníti meg az állapotot, és HTTP-n keresztül szinkronizál egy backend szerverrel.

## Build & Feltöltés

Ez egy Arduino IDE projekt (`.ino` fájl), nincs CLI build rendszer. Fordításhoz és feltöltéshez:
- Nyisd meg a `thermostat_01.ino` fájlt Arduino IDE-ben (vagy arduino-cli-vel)
- Board: **ESP8266** (pl. NodeMCU vagy Wemos D1)
- Szükséges könyvtárak: `ESP8266WiFi`, `ESP8266HTTPClient`, `ArduinoJson`, `SSD1306` (ESP8266 OLED driver), `OneWire`, `DallasTemperature`, `Ticker`

`arduino-cli` használatával:
```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 thermostat_01.ino
arduino-cli upload  --fqbn esp8266:esp8266:nodemcuv2 -p /dev/ttyUSB0 thermostat_01.ino
```

## Hardware pin kiosztás

| Define | GPIO | Funkció |
|--------|------|---------|
| ONE_WIRE_BUS | 0 | DS18B20 hőmérséklet szenzor |
| PIN_CLK | 14 | Rotary encoder órajel |
| PIN_DT | 12 | Rotary encoder adat |
| PIN_SW | 13 | Rotary encoder nyomógomb |
| PIN_RELAY | 15 | Relé (fűtésvezérlés) |
| PIN_SCL | 4 | I2C SCL (OLED kijelző) |
| PIN_SDA | 5 | I2C SDA (OLED kijelző) |

## Architektúra

Egyetlen `.ino` fájl. Kulcsminta: **megszakítás + flag-alapú főhurok**.

- **Ticker timerek** ISR-biztos callbackeket hívnak, amelyek csak `volatile boolean` flageket állítanak be (`flagSend`, `flagGet`, `flagCurrentTemp`)
- **`loop()`** ellenőrzi a flageket, és elvégzi a tényleges munkát (WiFi HTTP hívások, szenzor olvasás, kijelző frissítés) — elkerülve az I/O műveletek ISR-ben való végrehajtását
- **Rotary encoder** növeli/csökkenti a `virtualPosition` értéket (→ `setTemp = virtualPosition * 0.5`), és `flagManual` módba kapcsol
- **Auto mód**: a szervertől kapott `setTempAuto` lesz a `setTemp`; manuális módban az encoder értéke
- **Relé logika**: relé BE, ha `currentTemp < (setTemp + delta)`

## Szerver API

A backend a `http://thermo` címen érhető el (helyi hálózat). Az adatokat HTTP PUT kéréssel küldi a `/insert` végpontra JSON formátumban: `currTemp`, `adjustTemp`, `circoState`, `controlMode` mezőkkel. A GET oldal (`flagGet`) még nincs implementálva.

## Ismert folyamatban lévő dolgok

- WiFi hitelesítő adatok (`ssid`/`password`) hardkódolva vannak a forrásban
- A `flagGet` kezelője üres — a szerverről való lekérdezés még nincs megvalósítva
- A `flagSend` jelenleg hardkódolt tesztértékeket küld valódi szenzorleolvasás helyett
- A kikommentezett kód egy korábbi GET-alapú API-ról való átállást mutat PUT/JSON-ra
