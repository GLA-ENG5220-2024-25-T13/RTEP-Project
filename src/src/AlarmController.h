#ifndef ALARMCONTROLLER_H
#define ALARMCONTROLLER_H

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>

enum class AlarmState
{
    DISARMED,
    ARMED,
    TRIGGERED
};

class AlarmController
{
public:
    AlarmController(std::string alertSoundPath = "",
                    std::string playCmd = "",
                    std::string stopCmd = "");

    void arm();
    void disarm();
    void trigger(const std::string &source); // source: "PIR", "PROXIMITY"
    void resetTrigger();                     // Manually reset from TRIGGERED to ARMED

    AlarmState getState() const;
    std::string getStateString() const;
    std::string getLastTriggerSource() const;

    // methods/members for sensor status
    bool isPirActive() const;
    bool isProximityActive() const;
    // ------

    // For thread synchronization
    std::mutex &getMutex();
    std::condition_variable &getConditionVariable();
    bool isArmed() const;

private:
    void playAlertSound();
    void stopAlertSound();

    std::atomic<AlarmState> currentState;
    mutable std::mutex stateMutex; // Mutable to allow locking in const methods like getState
    std::condition_variable stateCv;
    std::string lastTriggerSource;

    // --- New members for active sensor tracking ---
    std::atomic<bool> pirTriggerActive;
    std::atomic<bool> proximityTriggerActive;
    // --- End New ---

    // --- Sound configuration ---
    std::string soundFilePath;
    std::string soundPlayCommand; // e.g., "aplay" or "mpg123"
    std::string soundStopCommand; // e.g., "pkill aplay" or "pkill mpg123"
    // --- End Sound config ---
};

#endif