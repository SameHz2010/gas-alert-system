#pragma once

#include <time.h>

struct RemoteSnapshot
{
    String deviceId;
    String timeText;
    unsigned long epoch;
    float temperature;
    float humidity;
    int gas;
    float deltaGas;
    float gasRelative;
    int state;
    bool valid;
};

bool initWiFi();
bool initTime();
void initFirebase();
void uploadData(float temperature, float humidity, int gas, float deltaGas, float gasRelative, int state, const tm &info);
