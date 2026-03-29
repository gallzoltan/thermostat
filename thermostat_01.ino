#include "config.h"
#include "sensor.h"
#include "display.h"
#include "network.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>

Ticker tCurrTemp;
Ticker tSendData;
Ticker tGetData;

volatile int     virtualPosition = 40;
volatile boolean flagSend        = false;
volatile boolean flagGet         = false;
volatile boolean flagTurn        = false;
volatile boolean flagPressed     = false;
volatile boolean flagCurrentTemp = false;

boolean flagManual  = false;
boolean flagScreen  = false;
boolean relayState  = false;
float   currentTemp = 20;
float   setTempAuto = 20;
float   setTemp     = 20;

// --- ISR-ek ---

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

// --- Ticker callback-ek ---

void sendingData()   { flagSend        = true; }
void gettingData()   { flagGet         = true; }
void getCurrentTemp(){ flagCurrentTemp = true; }

// --- Setup ---

void setup() {
  pinMode(PIN_CLK,   INPUT);
  pinMode(PIN_DT,    INPUT);
  pinMode(PIN_SW,    INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);

  attachInterrupt(PIN_CLK, getTurned,  FALLING); // encoder forgatás
  attachInterrupt(PIN_SW,  getPressed, RISING);  // encoder gombnyomás

  tSendData.attach(30, sendingData);   // adatok küldése a szervernek 30 másodperc
  tGetData.attach( 60, gettingData);   // adatok kérése a szervertől 60 másodperc
  tCurrTemp.attach(10, getCurrentTemp);// aktuális hőmérséklet 10 másodperc

  Serial.begin(115200);
  delay(10);
  initSensor();
  delay(10);
  initDisplay();

  drawMessage("WiFi csatlakozas...");
  initWifi();
  delay(1000);

  drawScreen(currentTemp, setTemp, flagManual, relayState);
}

// --- Loop ---

void loop() {

  // volt gombnyomás? → vissza auto módba
  if (flagPressed) {
    flagPressed = false;
    flagManual  = false;
    flagScreen  = true;
    Serial.println("Opmode: auto");
  }

  // volt forgatás? → manuális mód, setTemp frissítés
  if (flagTurn) {
    flagTurn   = false;
    flagManual = true;
    flagScreen = true;

    if (virtualPosition > MAX_TEMP) virtualPosition = MAX_TEMP;
    if (virtualPosition < MIN_TEMP) virtualPosition = MIN_TEMP;
    setTemp = virtualPosition * 0.5f;
    Serial.println("Opmode: manual");
  }

  // hőmérséklet lekérdezés
  if (flagCurrentTemp) {
    flagCurrentTemp = false;
    flagScreen      = true;
    currentTemp     = getTemperature(currentTemp);
  }

  // szerver lekérdezés
  if (flagGet) {
    flagGet     = false;
    setTempAuto = getData(setTempAuto);
  }

  // szerver küldés
  if (flagSend) {
    flagSend   = false;
    flagScreen = true;
    sendData(currentTemp, setTemp, relayState, flagManual);
  }

  // auto módban szerver adja a célhőmérsékletet
  if (!flagManual) {
    setTemp         = setTempAuto;
    virtualPosition = int(setTemp * 2);
  }

  // relay hisztézis
  if (currentTemp < (setTemp - DELTA)) {
    relayState = true;
  } else if (currentTemp > (setTemp + DELTA)) {
    relayState = false;
  }
  digitalWrite(PIN_RELAY, relayState ? HIGH : LOW);

  if (flagScreen) {
    flagScreen = false;
    drawScreen(currentTemp, setTemp, flagManual, relayState);
  }
}
