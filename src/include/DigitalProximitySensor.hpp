#ifndef __DIGITAL_PROXIMITY_SENSOR
#define __DIGITAL_PROXIMITY_SENSOR

#include "SensorInterface.hpp"
#include <string>
#include <gpiod.hpp>
class DigitalProximitySensor : public SensorInterface
{
private:
    int pinNumber;  // Digital pin number
    bool lastState; // Last state, used to detect state changes
    std::string sensorId_;
    gpiod::chip chip;
    gpiod::line line;
    SensorCallback proximityDetectedCallback;
    SensorCallback proximityLostCallback;

public:
    DigitalProximitySensor(const std::string &sensorId, int pin)
        : sensorId_(sensorId), pinNumber(pin), lastState(false),
          proximityDetectedCallback(nullptr), proximityLostCallback(nullptr), chip("/dev/gpiochip0", gpiod::chip::OPEN_LOOKUP) {}
    bool initialize() override
    {
        // Initialize the digital pin(pi5 gpio via libgpiod)
        LOG(INFO) << "Initializing Digital Proximity Sensor (ID: " << sensorId_ << ") on pin " << pinNumber;
        line = chip.get_line(pinNumber);
        line.request(gpiod::line_request("proximity-sensor", gpiod::line_request::DIRECTION_INPUT, gpiod::line_request::FLAG_BIAS_PULL_DOWN));
        LOG(INFO) << "Digital Proximity Sensor (ID: " << sensorId_ << ") on pin " << pinNumber << " initialized successful";
        return true; // Assume initialization is successful
    }
    float readData() override
    {
        // Read the digital pin level (pi5 gpio via libgpiod)
        // The gpiod::line::get_value() function is used to read the high or low level of the corresponding pin, but it returns an integer type.  It's best to convert this to a boolean type.
        // 使用gpiod::line::get_value()将对应pin上的高低电平读出，但其返回的是int类型的值，我们最好将其转换为bool类型。
        bool currentState = (1 == line.get_value());
        return currentState ? 1.0f : 0.0f; // Return 1.0 for detected, 0.0 for not detected
    }
    // The implementation of the following 3 functions is the same as in PseudoDigitalProximitySensor.
    // 下面3个函数的的实现和PseudoDigitalProximitySensor中的一样
    void registerCallback(SensorEventType eventType, SensorCallback callback) override
    {
        if (eventType == SensorEventType::PROXIMITY_DETECTED)
        {
            proximityDetectedCallback = callback;
        }
        else if (eventType == SensorEventType::PROXIMITY_LOST)
        {
            proximityLostCallback = callback;
        }
        else
        {
            LOG(ERROR) << "Unsupported event type for DigitalProximitySensor: " << static_cast<int>(eventType);
        }
    }
    void unregisterCallback(SensorEventType eventType) override
    {
        if (eventType == SensorEventType::PROXIMITY_DETECTED)
        {
            proximityDetectedCallback = nullptr;
        }
        else if (eventType == SensorEventType::PROXIMITY_LOST)
        {
            proximityLostCallback = nullptr;
        }
    }
    void run() override
    {
        while (true)
        {
            bool currentState = static_cast<bool>(readData());
            if (currentState != lastState)
            {
                SensorEventData eventData;
                eventData.sensorId = sensorId_;
                eventData.value = currentState ? 1.0f : 0.0f;
                if (currentState)
                {
                    eventData.type = SensorEventType::PROXIMITY_DETECTED;
                    if (proximityDetectedCallback)
                    {
                        proximityDetectedCallback(eventData);
                    }
                }
                else
                {
                    eventData.type = SensorEventType::PROXIMITY_LOST;
                    if (proximityLostCallback)
                    {
                        proximityLostCallback(eventData);
                    }
                }
                lastState = currentState;
            }
        }
    }
    ~DigitalProximitySensor()
    {
        LOG(INFO) << "Sensor (ID: " << sensorId_ << ") releasing pin " << pinNumber;
        line.release();
        LOG(INFO) << "Pin (" << pinNumber << ") released";
    }
};

#endif