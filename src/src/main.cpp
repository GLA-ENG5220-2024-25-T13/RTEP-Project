#include "AlarmController.h"
#include "GpioHandler.h"
#include "I2cHandler.h"
#include "ApiServer.h"
#include <iostream>
#include <chrono>
#include <csignal> // For signal handling

#define VCNL4010_I2C_ADDR 0x13

// --- Global Pointers/References for Signal Handler ---
// Use atomic or proper locking if accessed from signal handler directly,
// but it's safer to just set a flag for the main loop.
std::atomic<bool> keepRunning(true);

void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    keepRunning.store(false);
    // Potentially notify condition variables if threads are waiting indefinitely
    // Example: alarmControllerInstance->getConditionVariable().notify_all();
    // Be cautious with complex operations in signal handlers.
}

int main()
{
    std::cout << "Starting Alarm System..." << std::endl;

    // --- Configuration ---
    const std::string GPIO_CHIP = "gpiochip0";       // RPi 5 uses gpiochip0 for header pins
    const unsigned int PIR_GPIO_LINE = 17;           // GPIO 17 for PIR output
    const std::string I2C_DEVICE = "/dev/i2c-1";     // I2C bus 1 on RPi header
    const uint8_t VCNL4010_ADDR = VCNL4010_I2C_ADDR; // 0x13
    const int I2C_POLL_INTERVAL_MS = 150;            // How often to check proximity sensor
    const uint16_t PROXIMITY_THRESHOLD = 4000;       // Adjust based on testing
    const std::string API_HOST = "0.0.0.0";          // Listen on all interfaces
    const int API_PORT = 8080;                       // API server port

    // --- Sound Configuration ---
    const std::string ALARM_SOUND_FILE = "./alarm.wav";
    const std::string SOUND_PLAYER_CMD = "mpv --loop=inf"; // Use mpv for play audio
    const std::string SOUND_STOP_CMD = "pkill mpv";        // Command to stop the player

    // --- Setup Signal Handling ---
    signal(SIGINT, signalHandler);  // Handle Ctrl+C
    signal(SIGTERM, signalHandler); // Handle kill command

    // --- Initialize Components ---
    AlarmController alarmController(ALARM_SOUND_FILE, SOUND_PLAYER_CMD, SOUND_STOP_CMD);

    GpioHandler gpioHandler(alarmController, GPIO_CHIP, PIR_GPIO_LINE);
    if (!gpioHandler.initialize())
    {
        std::cerr << "FATAL: Failed to initialize GPIO Handler." << std::endl;
        return 1;
    }

    I2cHandler i2cHandler(alarmController, I2C_DEVICE, VCNL4010_ADDR);
    if (!i2cHandler.initialize())
    {
        std::cerr << "FATAL: Failed to initialize I2C Handler." << std::endl;
        return 1;
    }

    ApiServer apiServer(alarmController, API_HOST, API_PORT);

    // --- Start Services ---
    gpioHandler.startMonitoring();
    i2cHandler.startMonitoring(I2C_POLL_INTERVAL_MS, PROXIMITY_THRESHOLD);
    if (!apiServer.start())
    {
        std::cerr << "FATAL: Failed to start API Server." << std::endl;
        // Stop already started threads before exiting
        gpioHandler.stopMonitoring();
        i2cHandler.stopMonitoring();
        return 1;
    }

    // --- Main Loop (Keep application alive) ---
    std::cout << "Alarm system running. Press Ctrl+C to exit." << std::endl;
    while (keepRunning.load())
    {
        // Main thread can sleep or perform other low-priority tasks
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Periodically print status
        // std::cout << "Current State: " << alarmController.getStateString() << std::endl;
    }

    // --- Shutdown Sequence ---
    std::cout << "Shutting down..." << std::endl;
    apiServer.stop();
    i2cHandler.stopMonitoring();
    gpioHandler.stopMonitoring(); // Important: Stop GPIO last if clean shutdown relies on it
    alarmController.disarm();     // Disarming ensures sound stop logic runs

    std::cout << "Alarm System stopped." << std::endl;
    return 0;
}