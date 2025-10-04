#ifndef __IMU_IMU_DATA_H__
#define __IMU_IMU_DATA_H__

#include <inttypes.h>
#include <Arduino.h>

namespace imu {

static const int ImuXyz = 3;
static const int ImuWxyz = 4;
static const int ImuWpjw = 3;
static const int ImuDataLen = 56;//4 + (4*3) + (4*3) + (4*4) + (4*3);

struct ImuData {
public:
    uint32_t timestamp;
    float acc[ImuXyz];
    float gyro[ImuXyz];
    float quat[ImuWxyz];
    float orientation[ImuWpjw]; //pitch jaw roll

    explicit ImuData() : timestamp(0) {
        memset(this, 0, ImuDataLen);
        quat[0] = 1.0F;
    }

    String getPitchString() {
        char buf[20];  // enough for 0–255
        std::sprintf(buf, "%i", (int)orientation[0]);
        return String(buf);
    }
    String getJawString() {
        char buf[20];  // enough for 0–255
        std::sprintf(buf, "%i", (int)orientation[1]);
        return String(buf);
    }
    String getRollString() {
        char buf[20];  // enough for 0–255
        std::sprintf(buf, "%i", (int)orientation[2]);
        return String(buf);
    }
};

} // imu

#endif // __IMU_IMU_DATA_H__
