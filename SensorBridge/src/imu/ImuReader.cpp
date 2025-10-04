#include "ImuReader.h"

namespace imu {
    ImuReader::ImuReader(m5::IMU_Class& m5) : m5Imu(m5), ahrs(), imuData() {
        memset(gyroOffsets, 0, sizeof(float) * ImuXyz);
    }

    bool ImuReader::initialize() {
        m5Imu.begin();
        return true;
    }

    bool ImuReader::writeGyroOffset(float x, float y, float z) {
        gyroOffsets[0] = x;
        gyroOffsets[1] = y;
        gyroOffsets[2] = z;
        return true;
    }

    bool ImuReader::update() {
        float& ax = imuData.acc[0];
        float& ay = imuData.acc[1];
        float& az = imuData.acc[2];
        float& gx = imuData.gyro[0];
        float& gy = imuData.gyro[1];
        float& gz = imuData.gyro[2];
        float& qw = imuData.quat[0];
        float& qx = imuData.quat[1];
        float& qy = imuData.quat[2];
        float& qz = imuData.quat[3];
        
        m5Imu.getAccel(&ax, &ay, &az);
        m5Imu.getGyro(&gx, &gy, &gz);

       // ax *= -1;
       // ay *= -1;
        
        gx -= gyroOffsets[0];
        gy -= gyroOffsets[1];
        gz -= gyroOffsets[2];

        ahrs.UpdateQuaternion(
            gx * DEG_TO_RAD, gy * DEG_TO_RAD,  gz * DEG_TO_RAD, 
            ax, ay, az,
            qw, qx, qy, qz);

        sampleCounter++;
        
        // update euler at 50 hz;

        if (sampleCounter % 4 == 0) {
            if (zeroRefSet) {
                // Conjugate of reference
                float cq0 = imuDataRef.quat[0];
                float cq1 = -imuDataRef.quat[1];
                float cq2 = -imuDataRef.quat[2];
                float cq3 = -imuDataRef.quat[3];

                // Relative quaternion = conj(q_ref) * q_raw
                float qr0, qr1, qr2, qr3;
                quatMultiply(cq0, cq1, cq2, cq3, imuData.quat[0], imuData.quat[1], imuData.quat[2], imuData.quat[3], qr0, qr1, qr2, qr3);

                QuaternionToEuler(qr0, qr1, qr2, qr3,  imuData.orientation[2], imuData.orientation[0], imuData.orientation[1]);
        
            } else {

                QuaternionToEuler(imuData.quat[0], imuData.quat[1], imuData.quat[2], imuData.quat[3],  imuData.orientation[2], imuData.orientation[0], imuData.orientation[1]);
            }
             imuData.orientation[1] *= -1;
        }

       /*          Serial.printf("Accel:  %.1f° \t %.1f° \t %.1f° \t Gyro:  %.1f° \t %.1f° \t %.1f° \t PitchJawRoll:  %.1f° \t %.1f° \t %.1f° \n",
                   imuData.acc[0], imuData.acc[1], imuData.acc[2], 
                   imuData.gyro[0],imuData.gyro[1],imuData.gyro[2],
                   imuData.orientation[0], imuData.orientation[1], imuData.orientation[2]);
                    //debugline
 */
        imuData.timestamp = millis();
        lastUpdated = imuData.timestamp;
        return true;
    }

    bool ImuReader::read(ImuData& outImuData) const {
        if (lastUpdated == outImuData.timestamp) {
            return false; // not updated
        }
        memcpy(&outImuData, &imuData, ImuDataLen);
        return true;
    }

    void ImuReader::setZero() {
        memcpy(&imuDataRef, &imuData, ImuDataLen);
        zeroRefSet = true;
    }
        
    void ImuReader::quatMultiply (
        float q0a, float q1a, float q2a, float q3a,
        float q0b, float q1b, float q2b, float q3b,
        float &q0, float &q1, float &q2, float &q3)
    {
        q0 = q0a*q0b - q1a*q1b - q2a*q2b - q3a*q3b;
        q1 = q0a*q1b + q1a*q0b + q2a*q3b - q3a*q2b;
        q2 = q0a*q2b - q1a*q3b + q2a*q0b + q3a*q1b;
        q3 = q0a*q3b + q1a*q2b - q2a*q1b + q3a*q0b;
    }

    void ImuReader::QuaternionToEuler(float& q0, float& q1, float& q2, float& q3,  float& pitch, float& roll, float& yaw) {
        /*
        pitch = asin(-2.0 * q1 * q3 + 2.0 * q0* q2);	// pitch
        roll  = atan2(2.0 * q2 * q3 + 2.0 * q0 * q1, -2.0 * q1 * q1 - 2.0 * q2* q2 + 1.0);	// roll
        yaw   = atan2(2.0*(q1*q2 + q0*q3),q0*q0+q1*q1-q2*q2-q3*q3);	//yaw

        pitch *= RAD_TO_DEG;
        yaw   *= RAD_TO_DEG;
        // Declination of SparkFun Electronics (40°05'26.6"N 105°11'05.9"W) is
        // 	8° 30' E  ± 0° 21' (or 8.5°) on 2016-07-19
        // - http://www.ngdc.noaa.gov/geomag-web/#declination
        //yaw   -= 8.5;
        roll  *= RAD_TO_DEG;
        */

        
            float sinr_cosp = 2.0f * (q0 * q1 + q2 * q3);
            float cosr_cosp = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
            roll = atan2(sinr_cosp, cosr_cosp);

            // pitch (y-axis rotation)
            /*
            float sinp = 2.0f * (q0 * q2 - q3 * q1);
            if (fabs(sinp) >= 1)
                pitch = copysign(M_PI / 2, sinp); // use 90° if out of range
            else
                pitch = asin(sinp);
*/
pitch = atan2(2.0f * (q0 * q2 + q1 * q3),
              1.0f - 2.0f * (q2 * q2 + q3 * q3));
              //fix to get -180 - 180 range
            // yaw (z-axis rotation)
            float siny_cosp = 2.0f * (q0 * q3 + q1 * q2);
            float cosy_cosp = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
            yaw = atan2(siny_cosp, cosy_cosp);         
        pitch *= RAD_TO_DEG;
        yaw   *= RAD_TO_DEG;   
        roll  *= RAD_TO_DEG;
        
    }


} // imu
