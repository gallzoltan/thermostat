#pragma once

void  initWifi();
void  sendData(float currentTemp, float setTemp, bool relayState, bool flagManual);
float getData(float currentSetTemp);
