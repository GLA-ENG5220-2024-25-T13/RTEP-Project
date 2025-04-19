#ifndef ALARMCONTROLLER_H
#define ALARMCONTROLLER_H

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>

enum class AlarmState {
    DISARMED,
    ARMED,
    TRIGGERED
};

class AlarmController {
public:
    AlarmController();

    void arm();
    void disarm();
    void trigger(const std::string& source); // source: "PIR", "PROXIMITY"
    void resetTrigger(); // Optional: Manually reset from TRIGGERED to ARMED/DISARMED

    AlarmState getState() const;
    std::string getStateString() const;
    std::string getLastTriggerSource() const;

    // For thread synchronization
    std::mutex& getMutex();
    std::condition_variable& getConditionVariable();
    bool isArmed() const;

private:
    std::atomic<AlarmState> currentState;
    mutable std::mutex stateMutex; // Mutable to allow locking in const methods like getState
    std::condition_variable stateCv;
    std::string lastTriggerSource;
};

#endif