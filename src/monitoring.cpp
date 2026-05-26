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
  constexpr int sampleCount = 15;
  int samples[sampleCount];

  // Lấy mẫu
  for (int i = 0; i < sampleCount; i++)
  {
    samples[i] = analogRead(MQ2A_PIN);
    delay(3);
  }

  // Bubble sort nhỏ gọn để lọc nhiễu
  for (int i = 0; i < sampleCount - 1; i++)
  {
    for (int j = i + 1; j < sampleCount; j++)
    {
      if (samples[i] > samples[j])
      {
        int temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }

  // Bỏ 2 giá trị nhỏ nhất + 2 lớn nhất
  long total = 0;
  for (int i = 2; i < sampleCount - 2; i++)
  {
    total += samples[i];
  }

  return total / (sampleCount - 4);
}

int detectGasState(float temperature, float humidity, int gas, float deltaGas, float gasRelative)
{
  bool harshEnv = (humidity > 90.0f || temperature > 50.0f || temperature < 0.0f);

  // Sensor có thể bị ảnh hưởng môi trường mạnh
  if (harshEnv && gas < GAS_WARNING_THRESHOLD)
    return 4;

  // Nguy hiểm thật sự
  if (gas >= GAS_DANGER_THRESHOLD && (deltaGas > 60.0f || gasRelative > 1.15f))
    return 3;

  // Cảnh báo mạnh
  if (gas >= GAS_WARNING_THRESHOLD && (deltaGas > 35.0f || gasRelative > 1.02f))
    return 2;

  // Cảnh báo nhẹ
  if (gas >= GAS_SAFE_THRESHOLD || deltaGas > 20.0f || gasRelative > 1.03f)
    return 1;

  return 0;
}

void alertControl(int state)
{
  static unsigned long lastBlink = 0;
  static bool ledState = false;

  // blink mỗi 500ms
  if (millis() - lastBlink >= 500)
  {
    lastBlink = millis();
    ledState = !ledState;
  }

  // State 2,3,4 thì LED nhấp nháy
  if (state == 2 || state == 3 || state == 4)
    digitalWrite(RED_LED, ledState ? HIGH : LOW);
  else
    digitalWrite(RED_LED, LOW);

  // Chỉ danger mới bật còi
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
