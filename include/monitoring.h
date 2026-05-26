#pragma once

bool initAlertModule();
void pollAlertModule();
int sampleGasSensor();
int detectGasState(float temperature, float humidity, int gas, float deltaGas, float gasRelative);
void alertControl(int state);
void sendStartupTestSms();
void sendAlertSms(int gasValue, unsigned long &lastSmsTime, int state);
void placeAlertCall(unsigned long &lastCallTime);
