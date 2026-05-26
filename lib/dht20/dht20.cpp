#include "dht20.h"

void i2c_scan()
{
    Serial.println("\n----- I2C Scan -----");

    for (uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.print("Found device at 0x");
            Serial.println(addr, HEX);
        }
    }

    Serial.println("--------------------\n");
}

bool dht20_is_connected()
{
    Wire.beginTransmission(DHT20_ADDR);
    return (Wire.endTransmission() == 0);
}

void dht20_soft_reset()
{
    Wire.beginTransmission(DHT20_ADDR);
    Wire.write(0xBA);
    Wire.endTransmission();

    delay(20);
}

bool dht20_init()
{
    if (!dht20_is_connected())
    {
        Serial.println("DHT20 not found!");
        return false;
    }

    Serial.println("DHT20 detected");

    dht20_soft_reset();
    delay(100);

    return true;
}

bool dht20_read(float *temperature, float *humidity)
{
    uint8_t trigger_cmd[3] = {0xAC, 0x33, 0x00};

    Wire.beginTransmission(DHT20_ADDR);
    Wire.write(trigger_cmd, 3);
    if (Wire.endTransmission() != 0)
        return false;

    delay(80);

    Wire.requestFrom(DHT20_ADDR, 7);

    if (Wire.available() < 7)
        return false;

    uint8_t data[7];

    for (int i = 0; i < 7; i++)
        data[i] = Wire.read();

    if (data[0] & 0x80)
        return false;

    uint32_t RH_Code =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4) |
        (data[3] >> 4);

    uint32_t Temp_Code =
        (((uint32_t)data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        data[5];

    *humidity = ((float)RH_Code / 1048576.0) * 100.0;
    *temperature = ((float)Temp_Code / 1048576.0) * 200.0 - 50.0;

    return true;
}