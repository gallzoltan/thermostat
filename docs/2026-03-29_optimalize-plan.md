# Termosztát kód optimalizálás és hibajavítás

## Kontextus
Az egyetlen `thermostat_01.ino` fájlból álló ESP8266 termosztát kódban számos kritikus hiba, logikai probléma és hiányos implementáció van. A cél: a kód valóban működőképessé tétele (szerver kommunikáció, hőmérséklet szabályozás) és a stabilitási problémák megszüntetése.

---

## Javítások prioritás szerint

### 1. KRITIKUS: `http.end()` hiányzik → memóriaszivárgás
**Hely:** 181. sor után
Az `http.begin()` után soha nem hívódik meg `http.end()`. Minden 30 másodperces küldésnél erőforrás szivárog. Hosszabb üzemelés után a készülék lefagy.
**Javítás:** `http.end()` hívás hozzáadása a HTTP kérés után, hibás esetben is.

### 2. KRITIKUS: `getTemperature()` blokkoló végtelen loop timeout nélkül
**Hely:** 270-280. sor
Ha a DS18B20 szenzor hibásodik, a `do-while` végtelen loopba kerül és `delay(100)` blokkolja az egész rendszert (encoder, gomb, minden).
**Javítás:** Retry-számláló hozzáadása (pl. max 5 próba), ezután utolsó érvényes érték vagy hibajelzés visszaadása.

### 3. KRITIKUS: Buffer overflow a `currentTempString`-ben
**Hely:** 40. és 145. sor
`char currentTempString[6]` – ha a szenzor -127°C értéket ad vissza (hiba), a `dtostrf` 8 karaktert ír, ami stack corruptiont okoz.
**Javítás:** Méretet növelni `char currentTempString[8]`-ra.

### 4. MAGAS: Hisztézis logika hibás
**Hely:** 213-219. sor
Jelenlegi logika: relay BE ha `currentTemp < setTemp + delta`. Ez egyirányú – a relay be-kikapcsolása ugyanazon a küszöbön történik, ami gyors ciklusokat (relay „chattering") okoz.
**Javítás:** Klasszikus hisztézis: relay BE ha `currentTemp < setTemp - delta`, relay KI ha `currentTemp > setTemp + delta`. Ehhez egy `relayState` változó szükséges a jelenlegi állapot tárolásához.

### 5. MAGAS: `flagSend` valódi adatokat küldjön
**Hely:** 158-163. sor
Jelenleg hardkódolt `"23.3"`, `"21.1"`, `"0"`, `"1"` értékeket küld. A szerver értéktelen adatot kap.
**Javítás:** A payload-ban `currentTemp`, `setTemp`, `digitalRead(PIN_RELAY)`, és `flagManual ? 1 : 0` értékek szerepeljenek.

### 6. MAGAS: `flagGet` üres – szerver lekérdezés nincs implementálva
**Hely:** 149-151. sor
Az automatikus mód soha nem frissíti `setTempAuto`-t, mert a szerver válasz feldolgozása nincs megírva.
**Javítás:** HTTP GET kérés a szerverhez `/get` végpontra, a válasz JSON-ból `setTempAuto` frissítése. Az ArduinoJson v6 API-val: `StaticJsonDocument<200> doc; deserializeJson(doc, payload);`

### 7. KÖZEPES: ArduinoJson v5 → v6 API csere
**Hely:** 190-192. sor (kikommentezve)
A kommentált kódban `DynamicJsonBuffer` szerepel, ami ArduinoJson v5 API és a v6-tal nem fordul.
**Javítás:** `StaticJsonDocument<200>` és `deserializeJson()` használata a GET válasz feldolgozásánál.

### 8. KÖZEPES: WiFi reconnect logika hiányzik
**Hely:** `loop()` függvény
Ha a WiFi lecsatlakozik, az eszköz nem próbál újracsatlakozni.
**Javítás:** HTTP hívás előtt, ha `WiFi.status() != WL_CONNECTED`, `WiFi.reconnect()` hívása és rövid várakozás.

### 9. KISEBB: `virtualPosition` határértékek sorrendje
**Hely:** 130-136. sor
A `setTemp = virtualPosition*0.5` beállítás megtörténik, majd utána jön a határérték-ellenőrzés. Így egy lépéssel túl lehet menni.
**Javítás:** Határérték-ellenőrzés előbbre hozása, a `setTemp` beállítása után.

### 10. KISEBB: Ticker kommentek helytelenül írják az intervallumokat
**Hely:** 78. sor komment: `"adatok küldése a szervernek 10 perc"` – de a kód `30` másodperc.
**Javítás:** Komment javítása.

---

## Módosítandó fájl
- `thermostat_01.ino` (egyetlen fájl a projektben)

## Ellenőrzés
- Arduino IDE-ben vagy arduino-cli-vel lefordul-e hibák nélkül
- Soros monitoron (`115200 baud`) ellenőrizhető: WiFi csatlakozás, HTTP válasz kódok, hőmérséklet értékek
- Relay viselkedés: hőmérséklet szimulálásával ellenőrizhető a hisztézis
- Hosszabb üzemelés után a szabad memória (`ESP.getFreeHeap()`) monitorozható a szivárás ellenőrzéséhez
