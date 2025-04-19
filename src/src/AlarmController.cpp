#include "AlarmController.h"
#include <iostream>

AlarmController::AlarmController() : currentState(AlarmState::DISARMED), lastTriggerSource("None") {}

void AlarmController::arm() {
    std::lock_guard<std::mutex> lock(stateMutex);
    if (currentState == AlarmState::DISARMED) {
        currentState.store(AlarmState::ARMED);
        lastTriggerSource = "None";
        std::cout << "System ARMED" << std::endl;
        stateCv.notify_all(); // Notify potentially waiting threads
    }
}

void AlarmController::disarm() {
    std::lock_guard<std::mutex> lock(stateMutex);
    currentState.store(AlarmState::DISARMED);
    lastTriggerSource = "None";
    std::cout << "System DISARMED" << std::endl;
    stateCv.notify_all();
}

void AlarmController::trigger(const std::string& source) {
    std::lock_guard<std::mutex> lock(stateMutex);
    // Only trigger if currently armed
    if (currentState.load() == AlarmState::ARMED) {
        currentState.store(AlarmState::TRIGGERED);
        lastTriggerSource = source;
        std::cout << "ALARM TRIGGERED by " << source << "!" << std::endl;
        // Add actions here: sound siren, send notification, etc.
        stateCv.notify_all();
    }
}

void AlarmController::resetTrigger() {
     std::lock_guard<std::mutex> lock(stateMutex);
     if (currentState.load() == AlarmState::TRIGGERED) {
         // Decide whether to go back to ARMED or DISARMED
         currentState.store(AlarmState::ARMED); // Or DISARMED based on logic
         std::cout << "Alarm trigger reset. System back to " << getStateString() << std::endl;
         stateCv.notify_all();
     }
}


AlarmState AlarmController::getState() const {
    // No lock needed for atomic read, but keep for consistency if complex logic added
    return currentState.load();
}

bool AlarmController::isArmed() const {
     return getState() == AlarmState::ARMED;
}

std::string AlarmController::getStateString() const {
    AlarmState current = getState();
    switch (current) {
        case AlarmState::DISARMED: return "DISARMED";
        case AlarmState::ARMED:    return "ARMED";
        case AlarmState::TRIGGERED:return "TRIGGERED";
        default:                  return "UNKNOWN";
    }
}

std::string AlarmController::getLastTriggerSource() const {
     std::lock_guard<std::mutex> lock(stateMutex); // Protect access to lastTriggerSource
     return lastTriggerSource;
}


std::mutex& AlarmController::getMutex() {
    return stateMutex;
}

std::condition_variable& AlarmController::getConditionVariable() {
    return stateCv;
}