#include "network.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

void initWifi() {
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180); // 3 perc után folytatja AP nélkül is
  if (!wifiManager.autoConnect("Termostat")) {
    Serial.println("WiFi kapcsolat sikertelen, ujraindulas...");
    delay(3000);
    ESP.restart();
  }
  Serial.println("Connected to wifi");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
}

static bool wifiCheck() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi nincs, reconnect...");
    WiFi.reconnect();
    return false;
  }
  return true;
}

void sendData(float currentTemp, float setTemp, bool relayState, bool flagManual) {
  if (!wifiCheck()) return;

  StaticJsonDocument<200> doc;
  doc["currTemp"]    = String(currentTemp, 2);
  doc["adjustTemp"]  = String(setTemp, 2);
  doc["circoState"]  = relayState ? "1" : "0";
  doc["controlMode"] = flagManual  ? "1" : "0";

  String payload;
  serializeJson(doc, payload);
  Serial.println(payload);

  WiFiClient wifiClient;
  HTTPClient http;
  http.begin(wifiClient, String(SERVER_URL) + "/insert");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.sendRequest("PUT", payload);
  Serial.print("PUT: "); Serial.println(httpCode);
  http.end();
}

float getData(float currentSetTemp) {
  if (!wifiCheck()) return currentSetTemp;

  WiFiClient wifiClient;
  HTTPClient http;
  http.begin(wifiClient, String(SERVER_URL) + "/get");
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.print("GET valasz: "); Serial.println(payload);

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      float newTemp = doc["setterm"].as<float>();
      Serial.print("setTempAuto: "); Serial.println(newTemp);
      http.end();
      return newTemp;
    }
    Serial.print("JSON hiba: "); Serial.println(error.c_str());
  } else {
    Serial.print("GET hiba: "); Serial.println(httpCode);
  }

  http.end();
  return currentSetTemp;
}
