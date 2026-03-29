#include "sensor.h"
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>

static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature DS18B20(&oneWire);

void initSensor() {
  DS18B20.begin();
}

float getTemperature(float lastTemp) {
  float result = 0;
  int retries = 0;
  const int maxRetries = 5;

  do {
    DS18B20.requestTemperatures();
    result = DS18B20.getTempCByIndex(0);
    delay(100);
    retries++;
  } while ((result == 85.0f || result == -127.0f) && retries < maxRetries);

  if (retries >= maxRetries) {
    Serial.println("Szenzor hiba: nem sikerult ervenyes erteket olvasni!");
    return lastTemp;
  }

  return result;
}
