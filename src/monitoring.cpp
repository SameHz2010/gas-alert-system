#include <Arduino.h>
#include "secrets.h"
#include "config.h"
#include "monitoring.h"
#include "ST7789.h"

extern ST7789 lcd;

namespace
{
  HardwareSerial simSerial(2);
  String feedback;
  bool simReady = false;

  String readSIMFeedback(unsigned long timeoutMs = 1500, bool printIfEmpty = true)
  {
    unsigned long start = millis();
    feedback = "";

    while (millis() - start < timeoutMs)
    {
      while (simSerial.available())
      {
        char c = (char)simSerial.read();
        feedback += c;
      }
      delay(5);
    }

    if (feedback.length() > 0)
    {
      Serial.println(">> SIM Response:");
      Serial.println(feedback);
    }
    else if (printIfEmpty)
    {
      Serial.println(">> No response");
    }

    return feedback;
  }

  bool responseHasSuccess(const String &response)
  {
    return response.indexOf("OK") >= 0 || response.indexOf(">") >= 0;
  }

  bool sendAT(const String &cmd, unsigned long waitMs = 1000)
  {
    Serial.print("<< ");
    Serial.println(cmd);
    simSerial.print(cmd);
    simSerial.print("\r\n");
    delay(waitMs);

    String response = readSIMFeedback();
    return responseHasSuccess(response);
  }
} // namespace

bool initAlertModule()
{
  simSerial.begin(SIM_BAUD_RATE, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
  delay(1000);

  Serial.printf("Waiting for 4G signal on %lus...\n", SIM_BOOT_WAIT_MS / 1000UL);
  delay(SIM_BOOT_WAIT_MS);

  simReady = sendAT("AT");
  if (!simReady)
  {
    Serial.println("SIM module is not responding on UART.");
    return false;
  }

  simReady = true;
  sendAT("ATE0");
  sendAT("AT+CSCS=\"GSM\"");
  sendAT("AT+CMGF=1");
  sendAT("AT+CNMI=2,2,0,0,0");
  sendAT("AT+CMGD=1");
  sendAT("AT+CLIP=1");
  sendAT("AT&W");
  sendAT("AT+CSQ");

  return true;
}

void pollAlertModule()
{
  if (!simReady || !simSerial.available())
    return;

  feedback = "";
  while (simSerial.available())
  {
    feedback += (char)simSerial.read();
  }

  if (feedback.length() > 0)
  {
    Serial.println(">> Async SIM:");
    Serial.println(feedback);
  }
}

int sampleGasSensor()
{
  long total = 0;
  constexpr int sampleCount = 10;

  for (int i = 0; i < sampleCount; i++)
  {
    total += analogRead(MQ2A_PIN);
    delay(5);
  }

  return (int)(total / sampleCount);
}

int detectGasState(float temperature, float humidity, int gas, float deltaGas, float gasRelative)
{
  bool harshEnv = (humidity > 85.0f || temperature > 45.0f || temperature < 5.0f);

  if (harshEnv && gas < 900)
    return 4;

  // if (gas > GAS_DANGER_THRESHOLD && deltaGas > 80.0f && gasRelative > 1.20f)
  //   return 3;
  if (gas > 800)
    return 3;

  if (gas > (GAS_WARNING_THRESHOLD + 100) && (deltaGas > 40.0f || gasRelative > 1.10f))
    return 2;

  if (gas > (GAS_WARNING_THRESHOLD - 50) || deltaGas > 25.0f || gasRelative > 1.05f)
    return 1;

  return 0;
}

void alertControl(int state)
{
  digitalWrite(RED_LED, (state == 2 || state == 3 || state == 4) ? HIGH : LOW);
  digitalWrite(BUZZER, (state == 3) ? HIGH : LOW);
}

void sendStartupTestSms()
{
  if (!simReady)
  {
    Serial.println("Startup SMS skipped: SIM module is not ready");
    return;
  }

  Serial.println("<< Send startup SMS");
  simSerial.print("AT+CMGS=\"");
  simSerial.print(ALERT_PHONE_NUMBER);
  simSerial.print("\"\r");
  delay(500);

  String response = readSIMFeedback(1000);
  if (response.indexOf(">") < 0)
  {
    Serial.println("Startup SMS failed: SIM module did not enter message mode");
    return;
  }

  char startupMsg[96];
  snprintf(startupMsg, sizeof(startupMsg), "%s khoi dong he thong canh bao gas", DEVICE_ID);
  simSerial.print(startupMsg);
  delay(300);
  simSerial.write(26);
  delay(3000);
  readSIMFeedback(5000);
}

void sendAlertSms(int gasValue, unsigned long &lastSmsTime, int state)
{
  if (!simReady)
  {
    Serial.println("SIM SMS skipped: SIM module is not ready");
    return;
  }

  if (lastSmsTime != 0 && millis() - lastSmsTime < SMS_COOLDOWN)
    return;

  Serial.println("<< Send alert SMS");
  simSerial.print("AT+CMGS=\"");
  simSerial.print(ALERT_PHONE_NUMBER);
  simSerial.print("\"\r");
  delay(500);

  String response = readSIMFeedback(1000);
  if (response.indexOf(">") < 0)
  {
    Serial.println("Alert SMS failed: SIM module did not enter message mode");
    return;
  }

  char alertMsg[128];
  snprintf(alertMsg, sizeof(alertMsg), "CANH BAO GAS! %s State=%d Gas=%d", DEVICE_ID, state, gasValue);
  simSerial.print(alertMsg);
  delay(300);
  simSerial.write(26);
  delay(3000);
  readSIMFeedback(5000);

  lastSmsTime = millis();
}

void placeAlertCall(unsigned long &lastCallTime)
{
  if (!simReady)
  {
    Serial.println("SIM call skipped: SIM module is not ready");
    return;
  }

  if (millis() - lastCallTime < CALL_COOLDOWN)
    return;

  Serial.println("<< Call alert");
  simSerial.print("ATD");
  simSerial.print(ALERT_PHONE_NUMBER);
  simSerial.println(";");
  delay(500);
  readSIMFeedback(2000, false);

  delay(SIM_CALL_RING_MS);

  simSerial.println("AT+CHUP");
  delay(500);
  readSIMFeedback(2000, false);

  lastCallTime = millis();
}
