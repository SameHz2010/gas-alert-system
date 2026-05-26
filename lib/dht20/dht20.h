#ifndef DHT20_H
#define DHT20_H

#include <Arduino.h>
#include <Wire.h>

#define DHT20_ADDR 0x38

bool dht20_init();
bool dht20_read(float *temperature, float *humidity);
void dht20_soft_reset();
void i2c_scan();
bool dht20_is_connected();

#endif
