#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include "config.h"
#include "dht20.h"
#include "services.h"
#include "monitoring.h"
#include "ST7789.h"

#define MAX_POINTS 60 // 60 giây dữ liệu
#define STEP_X 4      // Mỗi giây cách nhau 4 pixel (4 * 60 = 240px)
#define WAVE_X 0
#define WAVE_Y 42
#define WAVE_W 240
#define WAVE_H 160 // Chiều cao vùng sóng

int gas_history[MAX_POINTS]; // Bộ đệm vòng lưu lịch sử gas
int head = 0;                // Con trỏ đầu bộ đệm
int p_count = 0;             // Số lượng điểm hiện có

ST7789 lcd(5, 2, 4, 15); // CS, DC, RST, BLK

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

// --- Hàm vẽ khung và lưới cho dạng sóng ---
void drawWaveformGrid()
{
  // Vẽ khung trắng
  lcd.fillRect(WAVE_X - 1, WAVE_Y - 1, WAVE_W + 2, WAVE_H + 2, COLOR_WHITE);
  // Vẽ nền đen bên trong
  lcd.fillRect(WAVE_X, WAVE_Y, WAVE_W, WAVE_H, COLOR_BLACK);

  // Vẽ lưới xám mờ (tùy chọn)
  for (int i = 1; i < 4; i++)
  {
    int y_grid = WAVE_Y + (WAVE_H * i / 4);
    lcd.drawLine(WAVE_X, y_grid, WAVE_X + WAVE_W, y_grid, COLOR_GRAY);
  }
}

void updateWaveform(int new_gas_val, int state, struct tm *timeinfo)
{
  // 1. Cập nhật dữ liệu vào bộ đệm vòng
  gas_history[head] = new_gas_val;
  head = (head + 1) % MAX_POINTS;
  if (p_count < MAX_POINTS)
    p_count++;

  // 2. Tìm Min-Max để Auto-Scale (Zoom biên độ)
  int min_val = 4095, max_val = 0;
  for (int i = 0; i < p_count; i++)
  {
    if (gas_history[i] < min_val)
      min_val = gas_history[i];
    if (gas_history[i] > max_val)
      max_val = gas_history[i];
  }

  // Zoom cực đại: Nếu gas ổn định, ép range tối thiểu 20 đơn vị để thấy rõ nhiễu sóng
  int range = (max_val - min_val < 20) ? 20 : (max_val - min_val);

  // 3. CHỈ XÓA VÙNG LÕI (Không xóa khung trắng và nhãn thời gian cố định)
  lcd.fillRect(WAVE_X, WAVE_Y, WAVE_W, WAVE_H, COLOR_BLACK);

  // Vẽ lưới mờ (mỗi 15s = 60px)
  for (int i = 60; i < 240; i += 60)
  {
    lcd.drawLine(i, WAVE_Y, i, WAVE_Y + WAVE_H, 0x1082); // Màu xám tối
  }

  // 4. LOGIC MÀU SẮC: Bình thường Vàng, Cảnh báo (S3) Đỏ
  uint16_t waveColor = (state >= 3) ? COLOR_RED : COLOR_YELLOW;

  // 5. Vẽ đường sóng (Làm dày 2 pixel để nhìn to và rõ hơn)
  for (int i = 0; i < p_count - 1; i++)
  {
    int idx0 = (head - p_count + i + MAX_POINTS) % MAX_POINTS;
    int idx1 = (head - p_count + i + 1 + MAX_POINTS) % MAX_POINTS;

    int x0 = i * STEP_X;
    int x1 = (i + 1) * STEP_X;

    // Tính toán tọa độ Y dựa trên Min-Max hiện tại
    int y0 = WAVE_Y + WAVE_H - ((gas_history[idx0] - min_val) * WAVE_H / range);
    int y1 = WAVE_Y + WAVE_H - ((gas_history[idx1] - min_val) * WAVE_H / range);

    lcd.drawLine(x0, y0, x1, y1, waveColor);
    lcd.drawLine(x0, y0 + 1, x1, y1 + 1, waveColor); // Vẽ thêm 1 đường để làm dày nét
  }

  // 6. Cập nhật số liệu Max/Min ở góc đồ thị (Xóa số cũ bằng màu nền đen)
  char buf[16];
  sprintf(buf, "MAX:%-4d", max_val);
  lcd.drawString(180, WAVE_Y + 5, buf, 0x7BEF, COLOR_BLACK);
  sprintf(buf, "MIN:%-4d", min_val);
  lcd.drawString(180, WAVE_Y + WAVE_H - 15, buf, 0x7BEF, COLOR_BLACK);

  // 7. Cập nhật Giờ NTP (Vùng cố định dưới cùng)
  if (timeinfo->tm_year > 0)
  {
    sprintf(buf, "%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    lcd.drawString(170, 220, buf, COLOR_CYAN, COLOR_BLACK);
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

  lcd.begin();
  lcd.fillScreen(COLOR_BLACK);

  lcd.drawRect(WAVE_X, WAVE_Y - 2, WAVE_W, WAVE_H + 4, COLOR_WHITE);
  lcd.drawLine(0, 212, 240, 212, COLOR_WHITE); // Đường trục X
  lcd.drawString(2, 220, "-60s", 0x7BEF, COLOR_BLACK);
  lcd.drawString(105, 220, "-30s", 0x7BEF, COLOR_BLACK);

  for (int i = 0; i < MAX_POINTS; i++)
    gas_history[i] = 0;
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

  static int last_state = -1;
  // Bình thường Vàng (Chữ Đen), Cảnh báo Đỏ (Chữ Trắng)
  uint16_t themeCol = (state >= 3) ? COLOR_RED : COLOR_YELLOW;
  uint16_t textCol = (state >= 3) ? COLOR_WHITE : COLOR_BLACK;

  if (state != last_state)
  {
    lcd.fillRect(0, 0, 240, 40, themeCol); // Chỉ xóa nền khi đổi trạng thái
    last_state = state;
  }

  char buf[32];
  // Cập nhật số liệu Gas và State
  sprintf(buf, "GAS: %-4d (S%d)", gas, state);
  lcd.drawString(10, 12, buf, textCol, themeCol);

  // --- THAY ĐỔI TẠI ĐÂY: Hiện 2 chữ số thập phân ---
  // %6.2f giúp cố định độ rộng để số không bị nhảy vị trí khi thay đổi
  sprintf(buf, "%.2fC|%.2f%%", temp, hum);

  // Có thể cần lùi tọa độ X sang trái một chút (từ 135 hoặc 145 về 125)
  // để đủ chỗ hiện thêm các chữ số thập phân
  lcd.drawString(125, 12, buf, textCol, themeCol);

  // --- Vẽ dạng sóng 60 giây ---
  updateWaveform(gas, state, &timeinfo);

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
