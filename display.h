#pragma once

void initDisplay();
void drawMessage(const char* msg);
void drawScreen(float currentTemp, float setTemp, bool flagManual, bool relayState);
