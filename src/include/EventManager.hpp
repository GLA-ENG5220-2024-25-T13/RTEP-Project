#ifndef __EVENT_MANAGER
#define __EVENT_MANAGER
#include "SensorInterface.hpp"
#include <map>      // std::map
#include <queue>    // std::queue
#include <mutex>    // std::mutex, std::lock_guard
#include <condition_variable> // std::condition_variable
#include <iostream>
#include <thread>

class EventManager {
public:
    using CallbackMap = std::map<SensorEventType, SensorCallback>;

    void registerSensor(std::shared_ptr<SensorInterface> sensor) {
        sensors_.push_back(sensor);
    }

    void registerCallbackForSensor(const std::string& sensorId, SensorEventType eventType, SensorCallback callback) {
        sensorCallbacks_[sensorId][eventType] = callback;
    }

    void unregisterCallbackForSensor(const std::string& sensorId, SensorEventType eventType) {
        sensorCallbacks_[sensorId].erase(eventType);
    }

    void postEvent(const SensorEventData& event) {
        {
            std::lock_guard<std::mutex> lock(eventQueueMutex_);
            eventQueue_.push(event);
        }
        eventQueueCondition_.notify_one(); // Notify event processing thread
    }

    void eventProcessingLoop() {
        while (true) {
            SensorEventData event;
            {
                std::unique_lock<std::mutex> lock(eventQueueMutex_);
                eventQueueCondition_.wait(lock, [this]{ return !eventQueue_.empty(); }); // Wait for non-empty event queue
                event = eventQueue_.front();
                eventQueue_.pop();
            }
            dispatch(event);
        }
    }

    void startSensorThreads() {
        for (auto& sensor : sensors_) {
            sensorThreads_.emplace_back([sensor, this]{
                sensor->registerCallback(SensorEventType::PROXIMITY_DETECTED, [this](const SensorEventData& event){ postEvent(event); });
                sensor->registerCallback(SensorEventType::PROXIMITY_LOST, [this](const SensorEventData& event){ postEvent(event); });
                sensor->registerCallback(SensorEventType::DISTANCE_CHANGED, [this](const SensorEventData& event){ postEvent(event); });
                sensor->run(); // Start sensor data reading loop
            });
        }
    }

    void startEventProcessingThread() {
        eventProcessingThread_ = std::thread([this]{ eventProcessingLoop(); });
    }

    void startSystem() {
        startSensorThreads();
        startEventProcessingThread();
        std::cout << "Alarm System Started." << std::endl;
    }

    void stopSystem() {
        // TODO: ... (Implement thread-safe stopping logic, e.g., set flag and join threads) ...
        std::cout << "Alarm System Stopped." << std::endl;
    }


private:
    std::vector<std::shared_ptr<SensorInterface>> sensors_;
    std::map<std::string, CallbackMap> sensorCallbacks_; // SensorID -> (EventType -> Callback function)
    std::queue<SensorEventData> eventQueue_;
    std::mutex eventQueueMutex_;
    std::condition_variable eventQueueCondition_;
    std::vector<std::thread> sensorThreads_;
    std::thread eventProcessingThread_;


    void dispatch(const SensorEventData& event) {
        auto sensorId = event.sensorId;
        auto eventType = event.type;

        if (sensorCallbacks_.count(sensorId) && sensorCallbacks_[sensorId].count(eventType)) {
            sensorCallbacks_[sensorId][eventType](event); // Call the registered callback function
        } else {
            std::cerr << "No callback registered for sensor " << sensorId << ", event type " << static_cast<int>(eventType) << std::endl;
        }
    }
};

#endif