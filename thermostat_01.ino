#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "SSD1306.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ticker.h>

#define ONE_WIRE_BUS 0

Ticker tCurrTemp;
Ticker tSendData;
volatile int virtualPosition=40;
volatile boolean flagSend = false;
volatile boolean flagTurn = false;
volatile boolean flagPressed = false;
volatile boolean flagCurrentTemp = false;

boolean flagManual = false;
boolean flagScreen = false;
float currentTemp = 20;
float setTempAuto = 20;
float setTemp = 20;
char currentTempString[6];
String server = "http://192.168.1.101/thermostat.php";

const char* ssid = "gallz";
const char* password = "ABizalomCsodakatTesz";
const float maxTemp = 64;
const float delta = 0.2;

const int PinCLK=14;                   // Used for generating interrupts using CLK signal
const int PinDT=12;                    // Used for reading DT signal
const int PinSW=13; 
const int PinRelay=15;

SSD1306  display(0x3c, 5, 4);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

void getTurned ()  {
  
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  flagTurn = true;
  
  if (interruptTime - lastInterruptTime > 10) {
    if(digitalRead(PinCLK) == digitalRead(PinDT)){
      virtualPosition++ ;
    }
    else {
      virtualPosition-- ;
    }
    
    lastInterruptTime = interruptTime;    
  }
}

void getPressed() {

  flagPressed = true;
}

void setup () {
 
  pinMode(PinCLK,INPUT);
  pinMode(PinDT,INPUT);  
  pinMode(PinSW,INPUT_PULLUP);
  pinMode(PinRelay,OUTPUT);
  
  attachInterrupt(PinCLK, getTurned, FALLING); // encoder turned
  attachInterrupt(PinSW, getPressed, RISING); // encoder pressed button
  tSendData.attach(60, sendingData); // adatok küldése szervernek 60mp
  tCurrTemp.attach(10, getCurrentTemp); // aktuális hőmérséklet 10 mp
  
  Serial.begin(115200);
  delay(10);
  DS18B20.begin();
  delay(10); 
  display.init();
  display.flipScreenVertically();

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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
  if(flagTurn){
    flagTurn = false;
    flagManual = true;
    flagScreen = true;
    setTemp = virtualPosition*0.5; 
    if(virtualPosition > maxTemp-1) {
      virtualPosition = maxTemp;
    }
    if(virtualPosition < 20){
      virtualPosition = 20;
    }
    Serial.println("Opmode: manual");
  }
  
  // le kell kérdezni az aktuális hőmérsékletet?
  if(flagCurrentTemp) {
    flagCurrentTemp = false;
    flagScreen = true;
    currentTemp = getTemperature();
    dtostrf(currentTemp, 2, 2, currentTempString);
  }

  // küldeni kell a szervernek?
  if (flagSend) {
      flagSend = false;
      flagScreen = true;
      String payload = "";
      String url = server;
      url += "?hello=OK";
      url += "&temp=";
      url += String(currentTempString);
      url += "&setterm=";
      url += String(setTemp);
      
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(url);
        int httpCode = http.GET();
     
        if (httpCode > 0) {
          payload = http.getString(); 
        }
     
        http.end();
    
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(payload);
        String setterm = root["setterm"];
        setTempAuto = setterm.toFloat();
        
        /*String getdata = root["getdata"];
        String setterm = root["setterm"];
        String settime = root["settime"];
        Serial.println(getdata);
        Serial.println(setterm);
        Serial.println(settime);*/
      }
      
      Serial.println(url);  
      Serial.println(payload);    
  }
  
  if(!flagManual){
    setTemp = setTempAuto;
    virtualPosition = int(setTemp*2);
    //sendRequest();
  }

  if(currentTemp < (setTemp+delta)){
    digitalWrite(PinRelay, HIGH);
  }
  else digitalWrite(PinRelay, LOW);
  /*if(currentTemp > setTemp){
    digitalWrite(PinRelay, LOW);
  }*/
  
  if(flagScreen) drawTest();
}

void drawTest() {

  flagScreen = false;
  int x=0;
  int y=0;  
  int ct = int(64 - ((currentTemp*100)-1080) / 30);
  int st = int(64 - ((setTemp*100)-1080) / 30);
  String curr = String(currentTempString);  // °C  
  String set = String(setTemp);   
  String myip = "IP: " + WiFi.localIP().toString();
  String opmode = (flagManual) ? "K" : "A";
  
  display.clear();
  //display.setContrast(50);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  
  display.setFont(ArialMT_Plain_24);  
  display.drawString(x+40, 15, curr);
  
  display.setFont(ArialMT_Plain_16);
  display.drawString(x+15, 46, set);

  display.setFont(ArialMT_Plain_10);
  display.drawString(x+120, y, opmode);

  display.drawLine(5, 64, 5, 0);
  display.drawLine(4, 64, 4, ct);
  display.drawLine(6, 64, 6, ct);
  display.drawLine(2, st, 9, st);
  
  display.display();
}

void sendingData() {
  flagSend = true;
}

void getCurrentTemp() {
  flagCurrentTemp = true;  
}

float getTemperature() {
  
  float result = 0;
  do {
    DS18B20.requestTemperatures(); 
    result = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (result == 85.0 || result == (-127.0));

  return result;
}
