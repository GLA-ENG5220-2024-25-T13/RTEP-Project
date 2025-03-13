#ifndef __SENSOR_INTERFACE
#define __SENSOR_INTERFACE
#include <glog/logging.h>
#include <functional> // std::function
#include <string>     // std::string

// Define sensor event types
enum class SensorEventType {
    PROXIMITY_DETECTED, // Proximity detected
    PROXIMITY_LOST,     // Proximity lost
    DISTANCE_CHANGED,   // Distance changed (specific to I2C sensors)
    ERROR               // Sensor error
};

// Define sensor event data structure
struct SensorEventData {
    SensorEventType type;
    std::string sensorId; // Sensor ID, used to distinguish between different sensors
    float value;          // Sensor value, e.g., distance value
};

// Define event callback function type
using SensorCallback = std::function<void(const SensorEventData&)>;

// Abstract sensor base class
class SensorInterface {
public:
    virtual ~SensorInterface() = default;

    // Initialize sensor
    virtual bool initialize() = 0;

    // Read sensor data (abstract method, implemented by specific sensor classes)
    virtual float readData() = 0;

    // Register event callback function
    virtual void registerCallback(SensorEventType eventType, SensorCallback callback) = 0;

    // Remove event callback function
    virtual void unregisterCallback(SensorEventType eventType) = 0;

    // Thread start run function
    virtual void run() = 0;

    // Get sensor ID
    virtual std::string getSensorId() const = 0;

    // Get sensor type (optional, used to distinguish sensor types)
    virtual std::string getSensorType() const = 0;
};

#endif