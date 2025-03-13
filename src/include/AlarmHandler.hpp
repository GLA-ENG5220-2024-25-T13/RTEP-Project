#ifndef __ALARM_HANDLER
#define __ALARM_HANDLER

#include"SensorInterface.hpp"
#include <iostream> // std::cout

class AlarmHandler {
public:
    void handleProximityDetected(const SensorEventData& event) {
        LOG(INFO) << "Proximity Detected by sensor " << event.sensorId << ", Value: " << event.value;
        // Add specific alarm logic here, such as playing a sound, sending a notification, etc.
        triggerAlarm();
    }

    void handleProximityLost(const SensorEventData& event) {
        LOG(INFO) << "Proximity Lost by sensor " << event.sensorId << ", Value: " << event.value;
        // Logic to cancel the alarm can be added
        stopAlarm();
    }

    void handleDistanceChanged(const SensorEventData& event) {
        // More refined alarm processing can be done based on distance changes
        LOG(INFO) << "Distance Changed by sensor " << event.sensorId << ", Value: " << event.value;
        if (event.value < alarmThresholdDistance) {
            triggerAlarm();
        } else {
            stopAlarm();
        }
    }

private:
    float alarmThresholdDistance = 5.0f; // Alarm distance threshold (e.g., 5cm)

    void triggerAlarm() {
        LOG(INFO)<< "** ALARM TRIGGERED! **";
        // ... Execute alarm actions, such as playing a sound, controlling indicator lights, etc. ...
    }

    void stopAlarm() {
        LOG(INFO)<< "** ALARM STOPPED. **";
        // ... Stop alarm actions ...
    }
};

#endif