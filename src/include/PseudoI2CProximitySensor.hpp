#include "SensorInterface.hpp"
#include <iostream> // std::cerr, std::cout
#include <thread>   // std::this_thread::sleep_for
#include <chrono>   // std::chrono::milliseconds

class PseudoI2CProximitySensor : public SensorInterface {
private:
    int i2cAddress; // I2C address
    float lastDistance; // Last distance value, used to detect changes
    SensorCallback proximityDetectedCallback;
    SensorCallback proximityLostCallback;
    SensorCallback distanceChangedCallback;

public:
    PseudoI2CProximitySensor(const std::string& sensorId, int address)
        : sensorId_(sensorId), i2cAddress(address), lastDistance(-1.0f), // Initialize to an invalid value
          proximityDetectedCallback(nullptr), proximityLostCallback(nullptr), distanceChangedCallback(nullptr) {}

    bool initialize() override {
        // Initialize I2C bus and sensor (initialization here depends on the specific I2C library and sensor model)
        // std::cout << "Initializing I2C Proximity Sensor (ID: " << sensorId_ << ") at address 0x" << std::hex << i2cAddress << std::dec << std::endl;
        
        // using glog:
        LOG(INFO)<< "Initializing I2C Proximity Sensor (ID: " << sensorId_ << ") at address 0x" << std::hex << i2cAddress << std::dec;
        // ... I2C initialization code ...
        return true; // Assume initialization is successful
    }

    float readData() override {
        // Read I2C sensor distance data (reading here depends on the specific I2C library and sensor model)
        float currentDistance = readI2CDistance(i2cAddress); // Assume there is a readI2CDistance function
        return currentDistance;
    }

    void registerCallback(SensorEventType eventType, SensorCallback callback) override {
        if (eventType == SensorEventType::PROXIMITY_DETECTED) {
            proximityDetectedCallback = callback;
        } else if (eventType == SensorEventType::PROXIMITY_LOST) {
            proximityLostCallback = callback;
        } else if (eventType == SensorEventType::DISTANCE_CHANGED) {
            distanceChangedCallback = callback;
        }
        else {
            // std::cerr << "Unsupported event type for PseudoI2CProximitySensor: " << static_cast<int>(eventType) << std::endl;
            LOG(ERROR)<< "Unsupported event type for PseudoI2CProximitySensor: " << static_cast<int>(eventType);
        }
    }

    void unregisterCallback(SensorEventType eventType) override {
        if (eventType == SensorEventType::PROXIMITY_DETECTED) {
            proximityDetectedCallback = nullptr;
        } else if (eventType == SensorEventType::PROXIMITY_LOST) {
            proximityLostCallback = nullptr;
        } else if (eventType == SensorEventType::DISTANCE_CHANGED) {
            distanceChangedCallback = nullptr;
        }
    }

    std::string getSensorId() const override {
        return sensorId_;
    }
    std::string getSensorType() const override {
        return "PseudoI2CProximitySensor";
    }

    void run() override {
        while (true) {
            float currentDistance = readData();
            if (currentDistance != lastDistance) {
                SensorEventData eventData;
                eventData.sensorId = sensorId_;
                eventData.value = currentDistance;
                eventData.type = SensorEventType::DISTANCE_CHANGED;
                if (distanceChangedCallback) {
                    distanceChangedCallback(eventData);
                }

                // Simple proximity judgment logic, the threshold can be adjusted according to sensor characteristics
                if (currentDistance < 10.0f) { // Assume distance less than 10cm is considered close
                    if (lastDistance >= 10.0f) { // Previously not close, now close, trigger PROXIMITY_DETECTED
                        eventData.type = SensorEventType::PROXIMITY_DETECTED;
                        if (proximityDetectedCallback) {
                            proximityDetectedCallback(eventData);
                        }
                    }
                } else {
                    if (lastDistance < 10.0f && lastDistance != -1.0f) { // Previously close, now far, trigger PROXIMITY_LOST
                        eventData.type = SensorEventType::PROXIMITY_LOST;
                        if (proximityLostCallback) {
                            proximityLostCallback(eventData);
                        }
                    }
                }
                lastDistance = currentDistance;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Sampling interval
        }
    }

private:
    std::string sensorId_;

    // Hypothetical I2C distance reading function, needs to be implemented based on the realworld sensor model
    float readI2CDistance(int address) {
        // ... I2C distance reading code ...
        // For demonstration and test purposes, simply simulate random distance values
        static float distance = 20.0f;
        static int count = 0;
        count++;
        if (count % 30 == 0) {
            distance += (rand() % 5 - 2) * 1.0f; // Randomly increase or decrease by -2cm to 2cm
            if (distance < 0.0f) distance = 0.0f;
            if (distance > 30.0f) distance = 30.0f;
        }
        return distance;
    }
};