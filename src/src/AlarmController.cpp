#include "AlarmController.h"
#include <iostream>

AlarmController::AlarmController(std::string alertSoundPath, std::string playCmd, std::string stopCmd)
    : currentState(AlarmState::DISARMED),
      lastTriggerSource("None"),
      pirTriggerActive(false),       // Initialize new members
      proximityTriggerActive(false), // Initialize new members
      soundFilePath(alertSoundPath),
      soundPlayCommand(playCmd),
      soundStopCommand(stopCmd)
{
}

// --- Sound Play/Stop Methods ---
void AlarmController::playAlertSound()
{
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
}

void AlarmController::stopAlertSound()
{
    // WARNING: Using pkill/killall is broad. If other instances of the player
    // are running for different reasons, they will also be stopped.
    // A more precise method involves tracking the PID from fork/exec.
    std::cout << "Executing sound stop command: " << soundStopCommand << std::endl;
    int ret = system(soundStopCommand.c_str());
    if (ret != 0)
    {
        std::cerr << "Warning: Sound stop command execution might have failed (return code: " << ret << ")" << std::endl;
    }
}
// --- End Sound Methods ---

void AlarmController::arm()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (currentState == AlarmState::DISARMED)
    {
        currentState.store(AlarmState::ARMED);
        lastTriggerSource = "None";
        pirTriggerActive.store(false);       // Reset sensor states
        proximityTriggerActive.store(false); // Reset sensor states
        std::cout << "System ARMED" << std::endl;
        stateCv.notify_all(); // Notify potentially waiting threads
        stopAlertSound();     // Ensure sound is stopped if it was somehow playing
    }
}

void AlarmController::disarm()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    bool wasTriggered = (currentState.load() == AlarmState::TRIGGERED);
    currentState.store(AlarmState::DISARMED);
    lastTriggerSource = "None";
    pirTriggerActive.store(false);       // Reset sensor states
    proximityTriggerActive.store(false); // Reset sensor states
    std::cout << "System DISARMED" << std::endl;
    if (wasTriggered)
    {
        stopAlertSound(); // Stop the alert sound
    }
    stateCv.notify_all();
}

void AlarmController::trigger(const std::string &source)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    bool alreadyTriggered = (currentState.load() == AlarmState::TRIGGERED);

    // Only trigger if currently armed
    if (currentState.load() == AlarmState::ARMED || alreadyTriggered)
    {
        // Set individual sensor state regardless of overall state change
        if (source == "PIR")
        {
            pirTriggerActive.store(true);
        }
        else if (source == "PROXIMITY")
        {
            proximityTriggerActive.store(true);
        }
        else
        {
            // TODO: Handle unknown source if necessary
        }

        // Transition to TRIGGERED state only if not already triggered
        if (!alreadyTriggered && currentState.load() == AlarmState::ARMED)
        {
            currentState.store(AlarmState::TRIGGERED);
            lastTriggerSource = source; // Store the *first* source
            std::cout << "ALARM TRIGGERED by " << source << "!" << std::endl;
            playAlertSound(); // Play the alert sound
            stateCv.notify_all();
        }
        else if (alreadyTriggered)
        {
            std::cout << "Additional trigger source detected: " << source << " (Alarm already active)" << std::endl;
            // Optional: Maybe restart the sound or log differently?
        }
    }
}

void AlarmController::resetTrigger()
{
    std::lock_guard<std::mutex> lock(stateMutex);
    if (currentState.load() == AlarmState::TRIGGERED)
    {
        // Reset to ARMED
        currentState.store(AlarmState::ARMED);
        pirTriggerActive.store(false);       // Reset sensor states
        proximityTriggerActive.store(false); // Reset sensor states
        lastTriggerSource = "Reset";         // Indicate manual reset
        std::cout << "Alarm trigger reset. System back to ARMED." << std::endl;
        stopAlertSound(); // Stop the alert sound
        stateCv.notify_all();
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
    std::lock_guard<std::mutex> lock(stateMutex); // Protect access to lastTriggerSource
    return lastTriggerSource;
}

std::mutex &AlarmController::getMutex()
{
    return stateMutex;
}

std::condition_variable &AlarmController::getConditionVariable()
{
    return stateCv;
}