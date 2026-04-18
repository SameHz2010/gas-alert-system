#pragma once

#include <time.h>

bool initWiFi();
bool initTime();
void initFirebase();
void uploadData(float temperature, float humidity, int gas, float deltaGas, float gasRelative, int state, const tm &info);
