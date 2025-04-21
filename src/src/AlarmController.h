#ifndef ALARMCONTROLLER_H
#define ALARMCONTROLLER_H

#ifdef RTEP_BUILD_WITH_GUI
#include <QObject>                                // Include QObject only when building GUI
#include <QString>                                // Include QString only when building GUI
#define RTEP_ALARMCONTROLLER_PARENT_CLASS QObject // Define parent class
#define RTEP_QOBJECT_MACRO Q_OBJECT               // Define Q_OBJECT macro
#else
#define RTEP_ALARMCONTROLLER_PARENT_CLASS public // Normal class without QObject
#define RTEP_QOBJECT_MACRO                       // Empty macro when not GUI
#endif

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

class AlarmController : RTEP_ALARMCONTROLLER_PARENT_CLASS
{
    RTEP_QOBJECT_MACRO // Use the conditional Q_OBJECT macro
public:
    AlarmController(std::string alertSoundPath = "",
                    std::string playCmd = "",
                    std::string stopCmd = ""
#ifdef RTEP_BUILD_WITH_GUI
                    , QObject *parent = nullptr // Add QObject parent only for GUI build
#endif);

#ifdef RTEP_BUILD_WITH_GUI
    Q_INVOKABLE void arm();
    Q_INVOKABLE void disarm();
    Q_INVOKABLE void resetTrigger();
#else
    void arm();
    void disarm();
    void resetTrigger();// Manually reset from TRIGGERED to ARMED
#endif
    void trigger(const std::string &source); // source: "PIR", "PROXIMITY"                  

    AlarmState getState() const;
#ifdef RTEP_BUILD_WITH_GUI
    QString getStateString() const;
    QString getLastTriggerSource() const;
#else
    std::string getStateString() const;
    std::string getLastTriggerSource() const;
#endif

    // methods/members for sensor status
    bool isPirActive() const;
    bool isProximityActive() const;
    // ------

    // For thread synchronization
    std::mutex &getMutex();
    std::condition_variable &getConditionVariable();
    bool isArmed() const;
#ifdef RTEP_BUILD_WITH_GUI
    signals:
        void stateChanged(AlarmState newState, const QString& stateString);
        void triggerSourceChanged(const QString& source);
        void sensorsUpdated(bool pirActive, bool proximityActive);
        void playAlarmSoundRequest(bool play);
#endif
private:
    void playAlertSound();
    void stopAlertSound();

    std::atomic<AlarmState> currentState;
    mutable std::mutex stateMutex; // Mutable to allow locking in const methods like getState
    std::condition_variable stateCv;
    std::string lastTriggerSource_std;

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