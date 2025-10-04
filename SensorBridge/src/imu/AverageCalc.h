#ifndef __IMU_AVERAGE_CALC_H__
#define __IMU_AVERAGE_CALC_H__

#include <Arduino.h>

namespace imu {

static const int DataMaxCount = 2000; //10 sec

class AverageCalc {
public:
    explicit AverageCalc();
    ~AverageCalc();
    bool push(float data);
    float average();
    int count()  { return cnt; }
    void reset() { 
        cnt = 0; 
        memset(data, 0, sizeof(float) * DataMaxCount);
    }
private:
    float data[DataMaxCount];
    int cnt;
};

class AverageCalcXYZ {
public:
    explicit AverageCalcXYZ() { }
    ~AverageCalcXYZ() { }
    bool push(float x, float y, float z) { return aveX.push(x) && aveY.push(y) && aveZ.push(z); }
    float averageX()  { return aveX.average(); }
    float averageY()  { return aveY.average(); }
    float averageZ()  { return aveZ.average(); }
    int countX()  { return aveX.count(); }
    int countY()  { return aveY.count(); }
    int countZ()  { return aveZ.count(); }
    void reset() { aveX.reset(); aveY.reset(); aveZ.reset();}
private:
    AverageCalc aveX;
    AverageCalc aveY;
    AverageCalc aveZ;
};

} // imu

#endif // __IMU_AVERAGE_CALC_H__
