#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ticker.h>

#define ONE_WIRE_BUS 0
#define PIN_CLK 14
#define PIN_DT 12
#define PIN_SW 13
#define PIN_RELAY 15
#define PIN_SCL 4
#define PIN_SDA 5

const char* ssid = "gallz";
const char* password = "ABizalomCsodakatTesz";
const float maxTemp = 64;
const float delta = 0.2;
const String server = "http://thermo";

Ticker tCurrTemp;
Ticker tSendData;
Ticker tGetData;

volatile int virtualPosition = 40;
volatile boolean flagSend = false;
volatile boolean flagGet = false;
volatile boolean flagTurn = false;
volatile boolean flagPressed = false;
volatile boolean flagCurrentTemp = false;

boolean flagManual = false;
boolean flagScreen = false;
boolean relayState = false;
float currentTemp = 20;
float setTempAuto = 20;
float setTemp = 20;
char currentTempString[8];  // -127.00 + null terminator

SSD1306  display(0x3c, PIN_SDA, PIN_SCL);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

void getTurned() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  flagTurn = true;

  if (interruptTime - lastInterruptTime > 10) {
    if (digitalRead(PIN_CLK) == digitalRead(PIN_DT)) {
      virtualPosition++;
    } else {
      virtualPosition--;
    }
    lastInterruptTime = interruptTime;
  }
}

void getPressed() {
  flagPressed = true;
}

void setup() {
  pinMode(PIN_CLK, INPUT);
  pinMode(PIN_DT, INPUT);
  pinMode(PIN_SW, INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);

  attachInterrupt(PIN_CLK, getTurned, FALLING);    // encoder forgatás
  attachInterrupt(PIN_SW, getPressed, RISING);      // encoder gombnyomás
  tSendData.attach(30, sendingData);                // adatok küldése a szervernek 30 másodperc
  tGetData.attach(60, gettingData);                 // adatok kérése a szervertől 60 másodperc
  tCurrTemp.attach(10, getCurrentTemp);             // aktuális hőmérséklet 10 másodperc

  Serial.begin(115200);
  delay(10);
  DS18B20.begin();
  delay(10);
  display.init();
  display.flipScreenVertically();

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to wifi");
  Serial.print("Status: "); Serial.println(WiFi.status());
  Serial.print("IP: ");     Serial.println(WiFi.localIP());
  Serial.print("Subnet: "); Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");Serial.println(WiFi.gatewayIP());
  Serial.print("SSID: ");   Serial.println(WiFi.SSID());
  Serial.print("Signal: "); Serial.println(WiFi.RSSI());
  delay(1000);

  display.clear();
  drawTest();
  display.display();
}

void loop() {

  // volt gomb nyomás?
  if (flagPressed) {
    flagPressed = false;
    flagManual = false;
    flagScreen = true;
    Serial.println("Opmode: auto");
  }

  // volt forgatás?
  if (flagTurn) {
    flagTurn = false;
    flagManual = true;
    flagScreen = true;

    // határérték ellenőrzés előbb, csak utána számítás
    if (virtualPosition > maxTemp) {
      virtualPosition = maxTemp;
    }
    if (virtualPosition < 20) {
      virtualPosition = 20;
    }
    setTemp = virtualPosition * 0.5;
    Serial.println("Opmode: manual");
  }

  // le kell kérdezni az aktuális hőmérsékletet?
  if (flagCurrentTemp) {
    flagCurrentTemp = false;
    flagScreen = true;
    currentTemp = getTemperature();
    dtostrf(currentTemp, 2, 2, currentTempString);
  }

  // lekérdezni a szervert?
  if (flagGet) {
    flagGet = false;

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient wifiClient;
      HTTPClient http;
      http.begin(wifiClient, server + "/get");
      int httpCode = http.GET();

      if (httpCode > 0) {
        String payload = http.getString();
        Serial.print("GET válasz: "); Serial.println(payload);

        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          setTempAuto = doc["setterm"].as<float>();
          Serial.print("setTempAuto: "); Serial.println(setTempAuto);
        } else {
          Serial.print("JSON hiba: "); Serial.println(error.c_str());
        }
      } else {
        Serial.print("GET hiba: "); Serial.println(httpCode);
      }
      http.end();
    } else {
      Serial.println("WiFi nincs, reconnect...");
      WiFi.reconnect();
    }
  }

  // küldeni kell a szervernek?
  if (flagSend) {
    flagSend = false;
    flagScreen = true;

    StaticJsonDocument<200> doc;
    doc["currTemp"]    = String(currentTemp, 2);
    doc["adjustTemp"]  = String(setTemp, 2);
    doc["circoState"]  = String(digitalRead(PIN_RELAY));
    doc["controlMode"] = flagManual ? "1" : "0";

    String payload;
    serializeJson(doc, payload);
    Serial.println(payload);

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient wifiClient;
      HTTPClient http;
      http.begin(wifiClient, server + "/insert");
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.sendRequest("PUT", payload);
      Serial.println(httpCode);
      http.end();
    } else {
      Serial.println("WiFi nincs, reconnect...");
      WiFi.reconnect();
    }
  }

  if (!flagManual) {
    setTemp = setTempAuto;
    virtualPosition = int(setTemp * 2);
  }

  // hisztézis: relay BE ha hőmérséklet az alsó küszöb alá esik, KI ha a felső fölé megy
  if (currentTemp < (setTemp - delta)) {
    relayState = true;
  } else if (currentTemp > (setTemp + delta)) {
    relayState = false;
  }
  digitalWrite(PIN_RELAY, relayState ? HIGH : LOW);

  if (flagScreen) drawTest();
}

void drawTest() {
  flagScreen = false;
  int x = 0;
  int y = 0;
  int ct = int(64 - ((currentTemp * 100) - 1080) / 30);
  int st = int(64 - ((setTemp * 100) - 1080) / 30);
  String curr = String(currentTempString);
  String set = String(setTemp, 1);
  String myip = "IP: " + WiFi.localIP().toString();
  String opmode = (flagManual) ? "K" : "A";

  display.clear();

  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.setFont(ArialMT_Plain_24);
  display.drawString(x + 40, 15, curr);

  display.setFont(ArialMT_Plain_16);
  display.drawString(x + 15, 46, set);

  display.setFont(ArialMT_Plain_10);
  display.drawString(x + 120, y, opmode);

  display.drawLine(5, 64, 5, 0);
  display.drawLine(4, 64, 4, ct);
  display.drawLine(6, 64, 6, ct);
  display.drawLine(2, st, 9, st);

  display.display();
}

void sendingData() {
  flagSend = true;
}

void gettingData() {
  flagGet = true;
}

void getCurrentTemp() {
  flagCurrentTemp = true;
}

float getTemperature() {
  float result = 0;
  int retries = 0;
  const int maxRetries = 5;

  do {
    DS18B20.requestTemperatures();
    result = DS18B20.getTempCByIndex(0);
    delay(100);
    retries++;
  } while ((result == 85.0 || result == (-127.0)) && retries < maxRetries);

  if (retries >= maxRetries) {
    Serial.println("Szenzor hiba: nem sikerult ervenyes erteket olvasni!");
    return currentTemp; // utolsó érvényes érték megtartása
  }

  return result;
}
