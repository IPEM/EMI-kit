#ifndef __IMU_IMU_READER_H__
#define __IMU_IMU_READER_H__

#include <M5Unified.h>
#include "utility/IMU_Class.hpp"
#include "mahony/MahonyAHRS.h"
#include "ImuData.h"

namespace imu {

class ImuReader {
public:
    explicit ImuReader(m5::IMU_Class& m5);
    bool initialize();
    bool writeGyroOffset(float x, float y, float z);
    bool update();
    bool read(ImuData& outImuData) const;
    
    void setZero();

    void QuaternionToEuler(float& q0, float& q1, float& q2, float& q3,  float& pitch, float& roll, float& yaw);
    
    void quatMultiply (
        float q0a, float q1a, float q2a, float q3a,
        float q0b, float q1b, float q2b, float q3b,
        float &q0, float &q1, float &q2, float &q3);

private:
    m5::IMU_Class& m5Imu;
    mahony::MahonyAHRS ahrs;
    ImuData imuData;
    ImuData imuDataRef;

    uint32_t lastUpdated;
    long sampleCounter = 0;
    float gyroOffsets[ImuXyz];

    float qref[4];  // reference quaternion
    bool zeroRefSet = false;
};

} // imu

#endif // __IMU_IMU_READER_H__
