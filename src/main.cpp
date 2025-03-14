#include "EventManager.hpp"
#include "PseudoDigitalProximitySensor.hpp"
#include "DigitalProximitySensor.hpp"
#include "PseudoI2CProximitySensor.hpp"
#include "AlarmHandler.hpp"
#include <memory> // std::shared_ptr, std::make_shared
int main(int argc, char **argv)
{
    EventManager eventManager;
    AlarmHandler alarmHandler;
    FLAGS_log_dir = "./logs/";
    FLAGS_alsologtostderr = true;
    FLAGS_colorlogtostderr = true;
    google::InitGoogleLogging(argv[0]);

    // Create sensor instances (assuming pin and I2C address)
    // auto digitalSensor1 = std::make_shared<PseudoDigitalProximitySensor>("PseudoDigitalSensor_1", 2);
    auto digitalSensor1 = std::make_shared<DigitalProximitySensor>("DigitalSensor_1", 17);
    auto i2cSensor1 = std::make_shared<PseudoI2CProximitySensor>("PseudoI2CSensor_1", 0x29);

    // Initialize sensors
    digitalSensor1->initialize();
    i2cSensor1->initialize();

    // Register sensors to the event manager
    eventManager.registerSensor(digitalSensor1);
    eventManager.registerSensor(i2cSensor1);

    // Register event callback functions
    eventManager.registerCallbackForSensor(digitalSensor1->getSensorId(), SensorEventType::PROXIMITY_DETECTED,
                                           [&alarmHandler](const SensorEventData &event)
                                           { alarmHandler.handleProximityDetected(event); });
    eventManager.registerCallbackForSensor(digitalSensor1->getSensorId(), SensorEventType::PROXIMITY_LOST,
                                           [&alarmHandler](const SensorEventData &event)
                                           { alarmHandler.handleProximityLost(event); });

    eventManager.registerCallbackForSensor(i2cSensor1->getSensorId(), SensorEventType::PROXIMITY_DETECTED,
                                           [&alarmHandler](const SensorEventData &event)
                                           { alarmHandler.handleProximityDetected(event); });
    eventManager.registerCallbackForSensor(i2cSensor1->getSensorId(), SensorEventType::PROXIMITY_LOST,
                                           [&alarmHandler](const SensorEventData &event)
                                           { alarmHandler.handleProximityLost(event); });
    eventManager.registerCallbackForSensor(i2cSensor1->getSensorId(), SensorEventType::DISTANCE_CHANGED,
                                           [&alarmHandler](const SensorEventData &event)
                                           { alarmHandler.handleDistanceChanged(event); });

    // Start the system
    eventManager.startSystem();

    // Let the main thread run for a while, simulating system operation
    std::this_thread::sleep_for(std::chrono::seconds(60));

    // Stop the system (a more graceful stop mechanism may be needed in actual applications)
    eventManager.stopSystem();

    return 0;
}