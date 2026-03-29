# Projekt szétbontása modulokra

## Kontextus
A teljes kód egyetlen `thermostat_01.ino` fájlban van. A cél: logikailag összefüggő részeket külön `.h`/`.cpp` fájlpárokba szervezni az átláthatóság és karbantarthatóság érdekében.

## Létrehozandó fájlok

### `config.h`
Pin definíciók és konstansok (`MAX_TEMP`, `DELTA`, `SERVER_URL`). Nincs `.cpp` párja.

### `sensor.h` + `sensor.cpp`
- `initSensor()` — DS18B20 inicializálás
- `getTemperature(float lastTemp)` — hőmérséklet olvasás, `lastTemp` visszaadása hiba esetén
- Az `OneWire` és `DallasTemperature` objektumok `static`-ként a `.cpp`-ben

### `display.h` + `display.cpp`
- `initDisplay()` — SSD1306 inicializálás
- `drawMessage(const char* msg)` — egyszerű szöveges üzenet (pl. WiFi csatlakozás közben)
- `drawScreen(float currentTemp, float setTemp, bool flagManual, bool relayState)` — főképernyő
- Az `SSD1306 display` objektum `static`-ként a `.cpp`-ben; a `currentTempString` konverzió itt történik, nem globalban

### `network.h` + `network.cpp`
- `initWifi()` — WiFiManager alapú csatlakozás
- `sendData(float currentTemp, float setTemp, bool relayState, bool flagManual)` — PUT /insert
- `getData(float currentSetTemp) -> float` — GET /get, visszaadja az új `setTempAuto`-t (vagy az előzőt hiba esetén)

### `thermostat_01.ino` (megmarad, leegyszerűsödik)
- Globális változók: `virtualPosition`, flagek, `currentTemp`, `setTempAuto`, `setTemp`, `relayState`
- ISR-ek: `getTurned()`, `getPressed()`
- Ticker callback-ek: `sendingData()`, `gettingData()`, `getCurrentTemp()`
- `setup()` és `loop()` — csak az üzleti logika, minden részlet a modulokban
- `currentTempString` **eltávolítva** — a `drawScreen` belsőleg végzi a konverziót

## Kritikus döntések
- `SERVER_URL` `config.h`-ban `#define`-ként, így a `network.cpp` `String(SERVER_URL) + "/endpoint"` formában használja (ODR-mentes)
- Minden modul-szintű objektum (`display`, `DS18B20`, `oneWire`) `static` a `.cpp`-ben — nem szivárog ki a globális névtérbe
- A `getTemperature()` szignatúrája `float lastTemp` paramétert kap — megszűnik a globális `currentTemp`-re való függés

## Ellenőrzés
- Arduino IDE-ben / arduino-cli-vel lefordul hibák nélkül
- Soros monitoron ellenőrizhető a WiFi csatlakozás, HTTP kódok, szenzor értékek
