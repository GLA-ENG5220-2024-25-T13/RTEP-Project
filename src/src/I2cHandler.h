#ifndef I2CHANDLER_H
#define I2CHANDLER_H

#include "AlarmController.h"
#include <string>
#include <thread>
#include <atomic>
#include <cstdint> // For uint16_t

class I2cHandler {
public:
    // Pass I2C device path (e.g., "/dev/i2c-1") and sensor address
    I2cHandler(AlarmController& controller, const std::string& devicePath, uint8_t deviceAddr);
    ~I2cHandler();

    bool initialize();
    void startMonitoring(int intervalMs = 200, uint16_t threshold = 3000); // Interval and proximity threshold
    void stopMonitoring();

private:
    void monitorLoop();
    bool readProximity(uint16_t& value);
    bool configureSensor(); // Helper to setup VCNL4010

    AlarmController& alarmController;
    std::string i2cDevicePath;
    uint8_t i2cDeviceAddr;
    int fd = -1; // File descriptor for I2C device

    int pollingIntervalMs;
    uint16_t proximityThreshold;

    std::thread monitorThread;
    std::atomic<bool> running;
};

#endif