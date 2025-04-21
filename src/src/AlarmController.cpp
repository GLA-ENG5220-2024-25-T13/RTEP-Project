#include "AlarmController.h"
#include <iostream> // Standard logging for non-GUI build

#ifdef RTEP_BUILD_WITH_GUI
#include <QDebug> // Qt logging for GUI build
#include <QString>
#endif

AlarmController::AlarmController(std::string alertSoundPath, std::string playCmd, std::string stopCmd
#ifdef RTEP_BUILD_WITH_GUI
                                 ,
                                 QObject *parent) : QObject(parent) // Call QObject constructor
#else
                                 )
#endif
    : currentState(AlarmState::DISARMED),
      lastTriggerSource_std("None"),
      pirTriggerActive(false),
      proximityTriggerActive(false),
      soundFilePath(alertSoundPath),
      soundPlayCommand(playCmd),
      soundStopCommand(stopCmd)
{
#ifdef RTEP_BUILD_WITH_GUI
    qInfo() << "AlarmController created (GUI Build)";
#else
    std::cout << "AlarmController created (Non-GUI Build)" << std::endl;
#endif
}

// --- Sound Play/Stop Methods ---
void AlarmController::playAlertSound()
{
#ifdef RTEP_BUILD_WITH_GUI
    qInfo() << "Requesting sound playback (GUI Build)";
    emit playAlarmSoundRequest(true);
#else
    // WARNING: Using system() is simple but has security risks and less control.
    // Consider using fork() and exec() for production code.
    // The '&' runs the command in the background.
    std::string command = soundPlayCommand + " " + soundFilePath + " &";
    std::cout << "Executing sound command: " << command << std::endl;
    int ret = system(command.c_str());
    if (ret != 0)
    {
        std::cerr << "Warning: Sound command execution might have failed (return code: " << ret << ")" << std::endl;
        // You might want more robust error checking here.
    }
#endif
}

void AlarmController::stopAlertSound()
{
#ifdef RTEP_BUILD_WITH_GUI
    qInfo() << "Requesting sound stop (GUI Build)";
    emit playAlarmSoundRequest(false);
#else
    // WARNING: Using pkill/killall is broad. If other instances of the player
    // are running for different reasons, they will also be stopped.
    // A more precise method involves tracking the PID from fork/exec.
    std::cout << "Executing sound stop command: " << soundStopCommand << std::endl;
    int ret = system(soundStopCommand.c_str());
    if (ret != 0)
    {
        std::cerr << "Warning: Sound stop command execution might have failed (return code: " << ret << ")" << std::endl;
    }
#endif
}
// --- End Sound Methods ---

void AlarmController::arm()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    AlarmState oldState = currentState.load();
    if (oldState == AlarmState::DISARMED)
    {
        currentState.store(AlarmState::ARMED);
        lastTriggerSource_std = "None";
        pirTriggerActive.store(false);
        proximityTriggerActive.store(false);
#ifdef RTEP_BUILD_WITH_GUI
        qInfo() << "System ARMED (GUI Build)";
        emit stateChanged(AlarmState::ARMED, getStateString());
        emit triggerSourceChanged(getLastTriggerSource());
        emit sensorsUpdated(isPirActive(), isProximityActive());
#else
        std::cout << "System ARMED (Non-GUI Build)" << std::endl;
#endif
        stopAlertSound();
    }
}

void AlarmController::disarm()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    AlarmState oldState = currentState.load();
    bool wasTriggered = (oldState == AlarmState::TRIGGERED);
    currentState.store(AlarmState::DISARMED);
    lastTriggerSource_std = "None";
    pirTriggerActive.store(false);
    proximityTriggerActive.store(false);
#ifdef RTEP_BUILD_WITH_GUI
    qInfo() << "System DISARMED (GUI Build)";
    emit stateChanged(AlarmState::DISARMED, getStateString());
    emit triggerSourceChanged(getLastTriggerSource());
    emit sensorsUpdated(isPirActive(), isProximityActive());
#else
    std::cout << "System DISARMED (Non-GUI Build)" << std::endl;
#endif
    if (wasTriggered)
    {
        stopAlertSound();
    }
}

void AlarmController::trigger(const std::string &source)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    AlarmState previousState = currentState.load();
    bool alreadyTriggered = (previousState == AlarmState::TRIGGERED);
    bool stateActuallyChanged = false;
    bool sensorStateChanged = false;

    if (previousState == AlarmState::ARMED || alreadyTriggered)
    {
        if (source == "PIR" && !pirTriggerActive.load())
        {
            pirTriggerActive.store(true);
            sensorStateChanged = true;
        }
        else if (source == "PROXIMITY" && !proximityTriggerActive.load())
        {
            proximityTriggerActive.store(true);
            sensorStateChanged = true;
        }

        if (!alreadyTriggered && previousState == AlarmState::ARMED)
        {
            currentState.store(AlarmState::TRIGGERED);
            lastTriggerSource_std = source;
            stateActuallyChanged = true;
#ifdef RTEP_BUILD_WITH_GUI
            qWarning() << "ALARM TRIGGERED by" << QString::fromStdString(source) << "!(GUI Build)";
#else
            std::cerr << "ALARM TRIGGERED by " << source << "! (Non-GUI Build)" << std::endl;
#endif
            playAlertSound();
        }
        else if (alreadyTriggered)
        {
#ifdef RTEP_BUILD_WITH_GUI
            qInfo() << "Additional trigger source detected:" << QString::fromStdString(source) << "(GUI Build)";
#else
            std::cout << "Additional trigger source detected: " << source << " (Non-GUI Build)" << std::endl;
#endif
        }
#ifdef RTEP_BUILD_WITH_GUI
        if (stateActuallyChanged)
        {
            emit stateChanged(AlarmState::TRIGGERED, getStateString());
            emit triggerSourceChanged(getLastTriggerSource());
        }
        if (sensorStateChanged || stateActuallyChanged)
        {
            emit sensorsUpdated(isPirActive(), isProximityActive());
        }
#endif
    }
    else
    { /* Trigger ignored */
    }
}

void AlarmController::resetTrigger()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    AlarmState oldState = currentState.load();
    if (oldState == AlarmState::TRIGGERED)
    {
        currentState.store(AlarmState::ARMED);
        lastTriggerSource_std = "Reset";
        pirTriggerActive.store(false);
        proximityTriggerActive.store(false);
#ifdef RTEP_BUILD_WITH_GUI
        qInfo() << "Alarm trigger reset. System back to ARMED (GUI Build)";
        emit stateChanged(AlarmState::ARMED, getStateString());
        emit triggerSourceChanged(getLastTriggerSource());
        emit sensorsUpdated(isPirActive(), isProximityActive());
#else
        std::cout << "Alarm trigger reset. System back to ARMED (Non-GUI Build)" << std::endl;
#endif
        stopAlertSound();
    }
}

bool AlarmController::isPirActive() const
{
    // Atomic load, no lock needed for simple read
    return pirTriggerActive.load();
}

bool AlarmController::isProximityActive() const
{
    // Atomic load, no lock needed for simple read
    return proximityTriggerActive.load();
}

AlarmState AlarmController::getState() const
{
    // No lock needed for atomic read, but keep for consistency if complex logic added
    return currentState.load();
}

bool AlarmController::isArmed() const
{
    return getState() == AlarmState::ARMED;
}

#ifdef RTEP_BUILD_WITH_GUI
QString AlarmController::getStateString() const
{
    AlarmState current = getState();
    switch (current)
    {
    case AlarmState::DISARMED:
        return QStringLiteral("DISARMED");
    case AlarmState::ARMED:
        return QStringLiteral("ARMED");
    case AlarmState::TRIGGERED:
        return QStringLiteral("TRIGGERED");
    default:
        return QStringLiteral("UNKNOWN");
    }
}
QString AlarmController::getLastTriggerSource() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return QString::fromStdString(lastTriggerSource_std);
}
#else
std::string AlarmController::getStateString() const
{
    AlarmState current = getState();
    switch (current)
    {
    case AlarmState::DISARMED:
        return "DISARMED";
    case AlarmState::ARMED:
        return "ARMED";
    case AlarmState::TRIGGERED:
        return "TRIGGERED";
    default:
        return "UNKNOWN";
    }
}
std::string AlarmController::getLastTriggerSource() const
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return lastTriggerSource_std;
}
#endif

std::mutex &AlarmController::getMutex()
{
    return stateMutex;
}

std::condition_variable &AlarmController::getConditionVariable()
{
    return stateCv;
}