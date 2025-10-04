# Sensor Bridge

## Current Limitations

The M5Unified IMU has several important limitations that affect its use for sensor fusion:

- **No sensor fusion**: The M5Unified library does not perform sensor fusion internally
- **No AHRS support**: Attitude and Heading Reference System (AHRS) functionality is not provided
- **Complex implementation**: Direct usage requires understanding of low-level IMU operations
- **No Magnetometer**: limits calibration options


## Technical References

### M5Unified IMU Implementation
- [IMU Class Header](https://raw.githubusercontent.com/m5stack/M5Unified/refs/heads/master/src/utility/IMU_Class.hpp) - Shows the raw IMU interface without fusion
- [M5StickC Plus2 IMU Documentation](https://docs.m5stack.com/en/arduino/m5stickc_plus2/imu) - Official usage guide
- [Sensor fusion link in old m5stick](https://github.com/m5stack/M5StickC-Plus/blob/master/src/utility/MahonyAHRS.h)

### Related Libraries
- [M5StickCPlus2 Repository](https://github.com/m5stack/M5StickCPlus2) - Wrapper around M5Unified
- [M5Unified Usage Example](https://github.com/m5stack/M5Unified/blob/master/examples/Basic/HowToUse/HowToUse.ino) - Demonstrates the complexity of direct usage

## Recommended Solutions

### Option 1: Adrduino AHRS Library
Use an Arduino compatible AHRS library to handle sensor fusion calculations:

- **GitHub Repository**: [Adafruit_AHRS](https://github.com/adafruit/Adafruit_AHRS/tree/master)
- **PlatformIO Installation**: [Library Registry](https://registry.platformio.org/libraries/adafruit/Adafruit%20AHRS/installation)
- https://github.com/Reefwing-Software/Reefwing-AHRS https://registry.platformio.org/libraries/reefwing-software/ReefwingAHRS
- **Benefits**: Well-tested, documented, and maintained sensor fusion algorithms

### Option 2: Custom Implementation
Develop a custom solution inspired by existing projects:

- **Reference Implementation**: [STERZStick main.cpp](https://github.com/Felixrising/STERZStick/blob/dev/src/main.cpp)
- **Benefits**: Full control over fusion algorithms, calibrations and performance optimization
- **Chosen Quick-Fix: use ahrs implementation from on m5stick (before unified)**: https://github.com/m5stack/M5StickC-Plus/blob/master/src/utility/MahonyAHRS.h


## Next Steps




1. Evaluate the Adafruit AHRS library for compatibility with M5Unified
2. Test performance characteristics of both approaches
3. Consider hybrid approach using Adafruit algorithms with custom optimizations