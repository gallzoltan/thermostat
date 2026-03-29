#include "display.h"
#include "config.h"
#include <SSD1306.h>
#include <Arduino.h>

static SSD1306 display(0x3c, PIN_SDA, PIN_SCL);

void initDisplay() {
  display.init();
  display.flipScreenVertically();
}

void drawMessage(const char* msg) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, msg);
  display.display();
}

void drawScreen(float currentTemp, float setTemp, bool flagManual, bool relayState) {
  char currentTempString[8];
  dtostrf(currentTemp, 2, 2, currentTempString);

  int ct = int(64 - ((currentTemp * 100) - 1080) / 30);
  int st = int(64 - ((setTemp * 100) - 1080) / 30);
  String opmode = flagManual ? "K" : "A";

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.setFont(ArialMT_Plain_24);
  display.drawString(40, 15, String(currentTempString));

  display.setFont(ArialMT_Plain_16);
  display.drawString(15, 46, String(setTemp, 1));

  display.setFont(ArialMT_Plain_10);
  display.drawString(120, 0, opmode);

  display.drawLine(5, 64, 5, 0);
  display.drawLine(4, 64, 4, ct);
  display.drawLine(6, 64, 6, ct);
  display.drawLine(2, st, 9, st);

  display.display();
}
