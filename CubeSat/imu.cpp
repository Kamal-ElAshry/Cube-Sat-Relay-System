#include "imu.h"
#include "config.h"

#include <Wire.h>

static bool _available = false;

bool imu_init() {
    Wire.begin();

    Wire.beginTransmission(MPU6050_I2C_ADDRESS);
    Wire.write(0x6B); // PWR_MGMT_1
    Wire.write(0x00); // wake the sensor up (it boots in sleep mode)
    uint8_t err = Wire.endTransmission();

    _available = (err == 0);
    return _available;
}

bool imu_isAvailable() {
    return _available;
}

bool imu_read(int16_t& ax, int16_t& ay, int16_t& az,
              int16_t& gx, int16_t& gy, int16_t& gz) {
    Wire.beginTransmission(MPU6050_I2C_ADDRESS);
    Wire.write(0x3B); // ACCEL_XOUT_H - start of the 14-byte sensor block
    uint8_t err = Wire.endTransmission(false); // repeated start
    if (err != 0) {
        _available = false;
        return false;
    }

    uint8_t got = Wire.requestFrom((int)MPU6050_I2C_ADDRESS, 14, (int)true);
    if (got < 14) {
        _available = false;
        return false;
    }

    ax = (int16_t)((Wire.read() << 8) | Wire.read());
    ay = (int16_t)((Wire.read() << 8) | Wire.read());
    az = (int16_t)((Wire.read() << 8) | Wire.read());
    Wire.read(); Wire.read(); // discard onboard temperature register (bytes 6-7)
    gx = (int16_t)((Wire.read() << 8) | Wire.read());
    gy = (int16_t)((Wire.read() << 8) | Wire.read());
    gz = (int16_t)((Wire.read() << 8) | Wire.read());

    _available = true;
    return true;
}
