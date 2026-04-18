#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include "config.h"
#include "dht20.h"
#include "services.h"
#include "monitoring.h"

float gas_prev = 0, gas_avg = 0;
unsigned long last_sms_time = 0;
unsigned long last_call_time = 0;
unsigned long last_loop_time = 0;
bool wifi_ready = false;
bool time_ready = false;
bool firebase_ready = false;
bool sim_ready = false;

void ensureServices()
{
  if (!wifi_ready)
    wifi_ready = initWiFi();

  if (wifi_ready && !time_ready)
    time_ready = initTime();

  if (wifi_ready && !firebase_ready)
  {
    initFirebase();
    firebase_ready = true;
  }
}

// MAIN SETUP VA LOOP
void setup()
{
  Serial.begin(115200);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  Wire.begin(21, 22);

  if (!dht20_init())
    Serial.println("DHT20 Fail");

  wifi_ready = initWiFi();
  if (wifi_ready)
  {
    time_ready = initTime();
    initFirebase();
    firebase_ready = true;
  }
  else
  {
    Serial.println("System will continue and retry connectivity in loop");
  }

  sim_ready = initAlertModule();
  if (SEND_TEST_SMS_ON_BOOT)
    sendStartupTestSms();
}

void loop()
{
  if (millis() - last_loop_time < LOOP_INTERVAL_MS)
    return;

  last_loop_time = millis();
  ensureServices();
  pollAlertModule();

  float temp, hum;
  int gas = sampleGasSensor();
  struct tm timeinfo;
  bool dht_ok = dht20_read(&temp, &hum);
  bool time_ok = getLocalTime(&timeinfo);

  float delta_gas = gas - gas_prev;
  gas_avg = (gas_avg * 0.9f) + (gas * 0.1f);
  float gas_relative = (gas_avg > 0) ? ((float)gas / gas_avg) : 1.0f;
  gas_prev = (float)gas;

  int state = dht_ok
                  ? detectGasState(temp, hum, gas, delta_gas, gas_relative)
                  : detectGasState(25.0f, 50.0f, gas, delta_gas, gas_relative);
  alertControl(state);

  if (sim_ready && (state == 3))
    sendAlertSms(gas, last_sms_time, state);

  // if (sim_ready && ENABLE_SIM_CALL_ON_DANGER && state == 3)
  //   placeAlertCall(last_call_time);

  if (dht_ok && time_ok)
  {
    Serial.println("THONG TIN CAM BIEN");

    Serial.printf("Temp         : %.2f C\n", temp);
    Serial.printf("Humidity     : %.2f %%\n", hum);
    Serial.printf("Gas          : %d\n", gas);
    Serial.printf("Delta Gas    : %.2f\n", delta_gas);
    Serial.printf("Gas Relative : %.2f\n", gas_relative);
    Serial.printf("State        : %d\n", state);
    Serial.println();

    uploadData(temp, hum, gas, delta_gas, gas_relative, state, timeinfo);
  }
}
