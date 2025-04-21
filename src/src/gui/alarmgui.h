#ifndef ALARMGUI_H
#define ALARMGUI_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtMultimedia/QSoundEffect> // For sound

// Forward declarations are fine here as cpp includes the full definition
#include "AlarmController.h" // Needs the definition for signal/slot connection and enum
class GpioHandler;
class I2cHandler;

class AlarmGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit AlarmGui(QWidget *parent = nullptr);
    ~AlarmGui();

protected:
    void closeEvent(QCloseEvent *event) override; // Handle window close

private slots:
    // Slots to receive signals from AlarmController
    void onStateChanged(AlarmState newState, const QString& stateString);
    void onTriggerSourceChanged(const QString& source);
    void onSensorsUpdated(bool pirActive, bool proximityActive);
    void handleAlarmSoundRequest(bool play);

    // Slots for button clicks
    void armSystem();
    void disarmSystem();
    void resetSystem();

private:
    void setupUi();
    bool initializeBackend(); // Helper to init controller and handlers
    void updateButtonStates(AlarmState currentState); // Update button enable/disable state
    void cleanupBackend(); // Stop threads, delete handlers

    // --- Configuration (Adapt these paths/values) ---
    // These constants define hardware/system specifics
    const std::string GPIO_CHIP = "gpiochip0";
    const unsigned int PIR_GPIO_LINE = 17;
    const std::string I2C_DEVICE = "/dev/i2c-1";
    const uint8_t VCNL4010_ADDR = 0x13; // From I2cHandler.cpp
    const int I2C_POLL_INTERVAL_MS = 150;
    const uint16_t PROXIMITY_THRESHOLD = 4000;
    const std::string ALARM_SOUND_FILE = "./alarm.wav"; // Relative path might need adjustment
    // --- End Configuration ---


    // UI Elements
    QLabel *statusLabel;
    QLabel *stateValueLabel;
    QLabel *triggerLabel;
    QLabel *triggerValueLabel;
    QLabel *pirSensorLabel;
    QLabel *pirSensorValueLabel;
    QLabel *proximitySensorLabel;
    QLabel *proximitySensorValueLabel;
    QLabel *infoLabel; // For general info/errors
    QPushButton *armButton;
    QPushButton *disarmButton;
    QPushButton *resetButton;
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;

    // Backend Components (Owned by the GUI)
    AlarmController *alarmController = nullptr;
    GpioHandler *gpioHandler = nullptr;
    I2cHandler *i2cHandler = nullptr;

    // Sound Effect Player
    QSoundEffect *alarmSound;
    bool backendInitialized = false;
};

#endif // ALARMGUI_H