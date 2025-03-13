#ifndef __PSEUDO_DIGITAL_SENSOR
#define __PSEUDO_DIGITAL_SENSOR

#include "SensorInterface.hpp"
#include <iostream> // std::cerr, std::cout
#include <thread>   // std::this_thread::sleep_for
#include <chrono>   // std::chrono::milliseconds

class PseudoDigitalProximitySensor : public SensorInterface {
private:
    int pinNumber; // Digital pin number
    bool lastState; // Last state, used to detect state changes
    SensorCallback proximityDetectedCallback;
    SensorCallback proximityLostCallback;

public:
    PseudoDigitalProximitySensor(const std::string& sensorId, int pin)
        : sensorId_(sensorId), pinNumber(pin), lastState(false),
          proximityDetectedCallback(nullptr), proximityLostCallback(nullptr) {}

    bool initialize() override {
        // Initialize the digital pin(pi5 gpio via libgpiod)
        // std::cout << "Initializing Digital Proximity Sensor (ID: " << sensorId_ << ") on pin " << pinNumber << std::endl;
        
        // using glog:
        LOG(INFO) << "Initializing Digital Proximity Sensor (ID: " << sensorId_ << ") on pin " << pinNumber;
        // ... Pin initialization code ...
        return true; // Assume initialization is successful
    }

    float readData() override {
        // Read the digital pin level (pi5 gpio via libgpiod)
        bool currentState = readDigitalPin(pinNumber); // Assume there is a readDigitalPin function
        return currentState ? 1.0f : 0.0f; // Return 1.0 for detected, 0.0 for not detected
    }

    void registerCallback(SensorEventType eventType, SensorCallback callback) override {
        if (eventType == SensorEventType::PROXIMITY_DETECTED) {
            proximityDetectedCallback = callback;
        } else if (eventType == SensorEventType::PROXIMITY_LOST) {
            proximityLostCallback = callback;
        } else {
            // std::cerr << "Unsupported event type for PseudoDigitalProximitySensor: " << static_cast<int>(eventType) << std::endl;
            // using glog instead: 
            LOG(ERROR)<< "Unsupported event type for PseudoDigitalProximitySensor: " << static_cast<int>(eventType);
        }
    }

    void unregisterCallback(SensorEventType eventType) override {
        if (eventType == SensorEventType::PROXIMITY_DETECTED) {
            proximityDetectedCallback = nullptr;
        } else if (eventType == SensorEventType::PROXIMITY_LOST) {
            proximityLostCallback = nullptr;
        }
    }

    std::string getSensorId() const override {
        return sensorId_;
    }
    std::string getSensorType() const override {
        return "PseudoDigitalProximitySensor";
    }

    void run() override {
        while (true) {
            bool currentState = static_cast<bool>(readData());
            if (currentState != lastState) {
                SensorEventData eventData;
                eventData.sensorId = sensorId_;
                eventData.value = currentState ? 1.0f : 0.0f;
                if (currentState) {
                    eventData.type = SensorEventType::PROXIMITY_DETECTED;
                    if (proximityDetectedCallback) {
                        proximityDetectedCallback(eventData);
                    }
                } else {
                    eventData.type = SensorEventType::PROXIMITY_LOST;
                    if (proximityLostCallback) {
                        proximityLostCallback(eventData);
                    }
                }
                lastState = currentState;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Sampling interval
        }
    }

private:
    std::string sensorId_;

    // Hypothetical digital pin reading function, needs to be implemented based on the hardware platform
    bool readDigitalPin(int pin) {
        // ... Hardware pin reading code ...
        // For demonstration and test purposes, simply simulate random state changes
        static bool state = false;
        static int count = 0;
        count++;
        if (count % 50 == 0) { // Switch state every once in a while
            state = !state;
        }
        return state;
    }
};

#endif