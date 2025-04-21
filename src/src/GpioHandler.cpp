#include "GpioHandler.h"
#include <iostream>
#include <chrono>
#include <errno.h>
#include <string.h>
#include <system_error> // For errno reporting

// --- Compatibility for libgpiod 1.x ---
// Helper to check libgpiod call results
bool check_gpiod_ret(int ret, const std::string &func_name)
{
    if (ret < 0)
    {
        std::cerr << "ERROR: libgpiod function " << func_name << " failed: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

GpioHandler::GpioHandler(AlarmController &controller, const std::string &chipName, unsigned int lineOffset)
    : alarmController(controller), gpioChipName(chipName), gpioLineOffset(lineOffset), running(false) {}

GpioHandler::~GpioHandler()
{
    stopMonitoring(); // Ensure thread is stopped
    if (line)
    {
        gpiod_line_release(line);
    }
    if (chip)
    {
        gpiod_chip_close(chip);
    }
}

bool GpioHandler::initialize()
{
    chip = gpiod_chip_open_by_name(gpioChipName.c_str());
    if (!chip)
    {
        std::cerr << "ERROR: Failed to open GPIO chip '" << gpioChipName << "': " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "Opened GPIO chip: " << gpiod_chip_name(chip) << std::endl;

    line = gpiod_chip_get_line(chip, gpioLineOffset);
    if (!line)
    {
        std::cerr << "ERROR: Failed to get GPIO line " << gpioLineOffset << ": " << strerror(errno) << std::endl;
        gpiod_chip_close(chip);
        chip = nullptr;
        return false;
    }
    std::cout << "Got GPIO line: " << gpiod_line_name(line) << " (offset " << gpiod_line_offset(line) << ")" << std::endl;

    // Request rising edge events (HC-SR501 usually goes HIGH on detection)
    // Use GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW if your sensor logic is inverted
    // For libgpiod 1.6.x, use gpiod_line_request_rising_edge_events_flags
    int ret = gpiod_line_request_rising_edge_events_flags(line, "alarm_system_pir", 0); // No special flags initially
    if (!check_gpiod_ret(ret, "gpiod_line_request_rising_edge_events_flags"))
    {
        gpiod_chip_close(chip);
        chip = nullptr;
        line = nullptr; // gpiod_line_release not needed if request failed early
        return false;
    }
    std::cout << "Requested rising edge events for GPIO " << gpioLineOffset << std::endl;

    return true;
}

void GpioHandler::startMonitoring()
{
    if (!line)
    {
        std::cerr << "ERROR: GPIO line not initialized. Cannot start monitoring." << std::endl;
        return;
    }
    if (running.load())
    {
        std::cout << "GPIO monitor already running." << std::endl;
        return;
    }
    running.store(true);
    monitorThread = std::thread(&GpioHandler::monitorLoop, this);
    std::cout << "GPIO monitoring thread started." << std::endl;
}

void GpioHandler::stopMonitoring()
{
    if (running.exchange(false))
    { // Atomically set running to false and check previous value
        std::cout << "Stopping GPIO monitoring thread..." << std::endl;
        // We need to wake up the gpiod_line_event_wait call.
        // Libgpiod doesn't have a direct cross-thread cancellation.
        // Options:
        // 1. Close the chip/line from another thread (might be risky).
        // 2. Use gpiod_line_event_wait_bulk with a timeout and check `running`.
        // 3. Send a signal (e.g., SIGUSR1) to the thread (more complex).
        // Let's choose option 2 (using wait with timeout).

        if (monitorThread.joinable())
        {
            monitorThread.join(); // This might block indefinitely if event_wait doesn't return.
                                  // A timeout version of wait is strongly recommended for robust shutdown.
            std::cout << "GPIO monitoring thread stopped." << std::endl;
        }
    }
}

void GpioHandler::monitorLoop()
{
    struct timespec timeout = {1, 0}; // 1 second timeout for wait, allows checking running flag
    struct gpiod_line_event event;    // Use gpiod_line_event for libgpiod >= 1.5

    while (running.load())
    {
        // Use gpiod_line_event_wait with timeout
        int ret = gpiod_line_event_wait(line, &timeout);

        if (ret < 0)
        { // Error
            std::cerr << "ERROR: gpiod_line_event_wait failed: " << strerror(errno) << std::endl;
            // Decide how to handle error (e.g., break loop, retry)
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Avoid busy-looping on error
            continue;
        }
        else if (ret == 0)
        { // Timeout
            // Timeout occurred, loop again to check running flag
            continue;
        }
        else
        {                                                       // Event occurred (ret == 1)
            int read_ret = gpiod_line_event_read(line, &event); // Read the event details
            if (!check_gpiod_ret(read_ret, "gpiod_line_event_read"))
            {
                // Handle read error
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            // We requested rising edge, so any event should be that.
            // But we could double-check event.event_type if needed (GPIOD_EDGE_EVENT_RISING_EDGE)
            std::cout << "GPIO Event Detected (Timestamp: " << event.ts.tv_sec << "." << event.ts.tv_nsec << ")" << std::endl;

            // Critical Section: Check alarm state and trigger if needed
            if (alarmController.isArmed())
            { // Quick check before locking
                alarmController.trigger("PIR");
            }
        }
    }
    std::cout << "GPIO monitor loop finished." << std::endl;
}