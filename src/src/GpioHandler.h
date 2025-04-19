#ifndef GPIOHANDLER_H
#define GPIOHANDLER_H

#include "AlarmController.h"
#include <string>
#include <thread>
#include <atomic>
#include <gpiod.h> // libgpiod C header

class GpioHandler {
public:
    GpioHandler(AlarmController& controller, const std::string& chipName, unsigned int lineOffset);
    ~GpioHandler();

    bool initialize();
    void startMonitoring();
    void stopMonitoring();

private:
    void monitorLoop();

    AlarmController& alarmController;
    std::string gpioChipName;
    unsigned int gpioLineOffset;

    struct gpiod_chip *chip = nullptr;
    struct gpiod_line *line = nullptr;

    std::thread monitorThread;
    std::atomic<bool> running;
};

#endif