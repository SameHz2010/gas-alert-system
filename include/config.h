#pragma once

#define MQ2A_PIN 34
#define MQ2D_PIN 35
#define RED_LED 26
#define BUZZER 27
#define SIM_RX_PIN 16
#define SIM_TX_PIN 17
#define DEVICE_ID "room_1"

constexpr unsigned long SMS_COOLDOWN = 300000UL;
constexpr unsigned long CALL_COOLDOWN = 600000UL;
constexpr unsigned long LOOP_INTERVAL_MS = 1000UL;
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000UL;
constexpr unsigned long NTP_SYNC_TIMEOUT_MS = 10000UL;
constexpr unsigned long SIM_BOOT_WAIT_MS = 15000UL;
constexpr unsigned long SIM_CALL_RING_MS = 10000UL;

constexpr uint32_t SIM_BAUD_RATE = 9600;

constexpr int GAS_WARNING_THRESHOLD = 700;
constexpr int GAS_DANGER_THRESHOLD = 1000;

constexpr bool SEND_TEST_SMS_ON_BOOT = true;
constexpr bool ENABLE_SIM_CALL_ON_DANGER = false;
