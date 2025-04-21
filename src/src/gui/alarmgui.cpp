#include "alarmgui.h"
#include "GpioHandler.h"   // Include actual definitions
#include "I2cHandler.h"    // Include actual definitions

#include <QtWidgets/QMessageBox>
#include <QtCore/QDebug>
#include <QtGui/QCloseEvent>
#include <QtCore/QUrl>

AlarmGui::AlarmGui(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(); // Create UI first

    // Initialize sound player
    alarmSound = new QSoundEffect(this);
    // Attempt to find the sound file relative to the application directory
    // This might need adjustment depending on deployment.
    QUrl soundUrl = QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/" + QString::fromStdString(ALARM_SOUND_FILE));

    qInfo() << "Attempting to load sound from:" << soundUrl.toString();

    if (soundUrl.isValid() && !soundUrl.isEmpty() && QFile::exists(soundUrl.toLocalFile())) {
        alarmSound->setSource(soundUrl);
        alarmSound->setLoopCount(QSoundEffect::Infinite); // Loop indefinitely
        alarmSound->setVolume(0.8); // Set volume (0.0 to 1.0)
        if(!alarmSound->isLoaded()){
             // This might happen asynchronously, check status later if needed
             qWarning() << "Sound file found but QSoundEffect reported not loaded immediately.";
        }
        qInfo() << "Sound effect created. Source:" << alarmSound->source().toString();
    } else {
        qWarning() << "Alarm sound file not found or path invalid:" << soundUrl.toLocalFile();
        infoLabel->setText("Warning: Alarm sound file not found.");
        infoLabel->setStyleSheet("color: orange;");
    }


    if (initializeBackend()) {
        backendInitialized = true;
         qInfo() << "Backend initialized successfully.";
         infoLabel->setText("System Ready.");
         infoLabel->setStyleSheet("color: green;");

        // Connect signals from controller to GUI slots
        // Use QueuedConnection for thread safety (signals likely from sensor threads)
        connect(alarmController, &AlarmController::stateChanged,
                this, &AlarmGui::onStateChanged, Qt::QueuedConnection);
        connect(alarmController, &AlarmController::triggerSourceChanged,
                this, &AlarmGui::onTriggerSourceChanged, Qt::QueuedConnection);
        connect(alarmController, &AlarmController::sensorsUpdated,
                this, &AlarmGui::onSensorsUpdated, Qt::QueuedConnection);
        connect(alarmController, &AlarmController::playAlarmSoundRequest,
                 this, &AlarmGui::handleAlarmSoundRequest, Qt::QueuedConnection);

        // Initial UI update based on controller's starting state
        onStateChanged(alarmController->getState(), alarmController->getStateString());
        onTriggerSourceChanged(alarmController->getLastTriggerSource());
        onSensorsUpdated(alarmController->isPirActive(), alarmController->isProximityActive());

        // Start monitoring threads *after* everything is set up
        gpioHandler->startMonitoring();
        i2cHandler->startMonitoring(I2C_POLL_INTERVAL_MS, PROXIMITY_THRESHOLD);

    } else {
        qCritical() << "Backend initialization failed!";
        QMessageBox::critical(this, "Initialization Error", "Failed to initialize backend components (GPIO/I2C). Check permissions and hardware.");
        // Disable UI elements if backend failed
        armButton->setEnabled(false);
        disarmButton->setEnabled(false);
        resetButton->setEnabled(false);
        infoLabel->setText("Error: Backend failed to initialize!");
        infoLabel->setStyleSheet("color: red;");
    }

    // Connect button clicks
    connect(armButton, &QPushButton::clicked, this, &AlarmGui::armSystem);
    connect(disarmButton, &QPushButton::clicked, this, &AlarmGui::disarmSystem);
    connect(resetButton, &QPushButton::clicked, this, &AlarmGui::resetSystem);
}

AlarmGui::~AlarmGui()
{
    qInfo() << "AlarmGui destructor called.";
    cleanupBackend(); // Ensure threads are stopped and objects deleted
    // Qt manages UI elements and alarmSound due to parentage
}

void AlarmGui::setupUi()
{
    setWindowTitle("RTEP Alarm Control (Integrated GUI)"); // Update title
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    mainLayout = new QVBoxLayout(centralWidget);

    statusLabel = new QLabel("Status:", centralWidget);
    stateValueLabel = new QLabel("<i>Initializing...</i>", centralWidget);
    triggerLabel = new QLabel("Last Trigger:", centralWidget);
    triggerValueLabel = new QLabel("<i>N/A</i>", centralWidget);
    pirSensorLabel = new QLabel("PIR Sensor:", centralWidget);
    pirSensorValueLabel = new QLabel("<i>N/A</i>", centralWidget);
    proximitySensorLabel = new QLabel("Proximity Sensor:", centralWidget);
    proximitySensorValueLabel = new QLabel("<i>N/A</i>", centralWidget);
    infoLabel = new QLabel("Initializing...", centralWidget);
    infoLabel->setStyleSheet("color: blue;");

    armButton = new QPushButton("Arm System", centralWidget);
    disarmButton = new QPushButton("Disarm System", centralWidget);
    resetButton = new QPushButton("Reset Trigger", centralWidget);

    // Add widgets to layout
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(stateValueLabel);
    mainLayout->addWidget(triggerLabel);
    mainLayout->addWidget(triggerValueLabel);
    mainLayout->addWidget(pirSensorLabel);
    mainLayout->addWidget(pirSensorValueLabel);
    mainLayout->addWidget(proximitySensorLabel);
    mainLayout->addWidget(proximitySensorValueLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(armButton);
    mainLayout->addWidget(disarmButton);
    mainLayout->addWidget(resetButton);
    mainLayout->addSpacing(20);
     mainLayout->addWidget(infoLabel);
    mainLayout->addStretch(1);

    // Initial button states (disabled until backend is confirmed)
    armButton->setEnabled(false);
    disarmButton->setEnabled(false);
    resetButton->setEnabled(false);

    resize(300, 400);
}

bool AlarmGui::initializeBackend()
{
    // 1. Create Controller (passes 'this' as parent for Qt memory management)
    // Sound command args are passed but won't be used by GUI build path inside controller
    alarmController = new AlarmController(ALARM_SOUND_FILE, "", "", this);

    // 2. Create Handlers, pass controller reference
    gpioHandler = new GpioHandler(*alarmController, GPIO_CHIP, PIR_GPIO_LINE);
    i2cHandler = new I2cHandler(*alarmController, I2C_DEVICE, VCNL4010_ADDR);

    // 3. Initialize Handlers
    if (!gpioHandler->initialize()) {
        qCritical() << "FATAL: Failed to initialize GPIO Handler.";
        cleanupBackend(); // Clean up partially created objects
        return false;
    }
    if (!i2cHandler->initialize()) {
        qCritical() << "FATAL: Failed to initialize I2C Handler.";
        cleanupBackend();
        return false;
    }

    return true; // Success
}

void AlarmGui::cleanupBackend()
{
    qInfo() << "Cleaning up backend...";
    if (i2cHandler) {
         qInfo() << "Stopping I2C monitoring...";
        i2cHandler->stopMonitoring();
    }
     if (gpioHandler) {
         qInfo() << "Stopping GPIO monitoring...";
        gpioHandler->stopMonitoring(); // Stop threads before deleting
    }

    // Delete in reverse order of creation (or safe order)
    // Qt parent system should handle deletion if parent was set correctly,
    // but explicit deletion after stopping threads is safer.
    delete i2cHandler; i2cHandler = nullptr;
    delete gpioHandler; gpioHandler = nullptr;
    // alarmController has 'this' as parent, Qt might delete it.
    // If managing manually: delete alarmController; alarmController = nullptr;
     qInfo() << "Backend cleanup complete.";
}

// --- Slots Implementation ---

void AlarmGui::onStateChanged(AlarmState newState, const QString& stateString)
{
     qInfo() << "GUI Slot: State changed to" << stateString;
    stateValueLabel->setText(stateString);

    // Update label style based on state
    if (newState == AlarmState::TRIGGERED) {
        stateValueLabel->setStyleSheet("color: red; font-weight: bold;");
    } else if (newState == AlarmState::ARMED) {
        stateValueLabel->setStyleSheet("color: orange; font-weight: bold;");
    } else { // DISARMED or UNKNOWN
        stateValueLabel->setStyleSheet("color: green;");
    }

    updateButtonStates(newState);
}

void AlarmGui::onTriggerSourceChanged(const QString& source)
{
     qInfo() << "GUI Slot: Trigger source changed to" << source;
    triggerValueLabel->setText(source);
}

void AlarmGui::onSensorsUpdated(bool pirActive, bool proximityActive)
{
    qInfo() << "GUI Slot: Sensors updated - PIR:" << pirActive << "Proximity:" << proximityActive;
    pirSensorValueLabel->setText(pirActive ? "<b style='color: red;'>Active</b>" : "Inactive");
    proximitySensorValueLabel->setText(proximityActive ? "<b style='color: red;'>Active</b>" : "Inactive");
}

void AlarmGui::handleAlarmSoundRequest(bool play)
{
    qInfo() << "GUI Slot: Sound request - Play:" << play;
    if (!alarmSound) return;

     if (!alarmSound->isLoaded()) {
          qWarning() << "Cannot play/stop sound, effect not loaded properly.";
          // Maybe try loading again? Or display persistent warning.
          // Example: Check if file exists again here.
          QUrl currentSource = alarmSound->source();
          if (!QFile::exists(currentSource.toLocalFile())) {
               infoLabel->setText("Error: Sound file missing!");
               infoLabel->setStyleSheet("color: red;");
          }
          return;
      }

    if (play) {
        if (alarmSound->status() != QSoundEffect::Playing) {
             alarmSound->play();
             qInfo() << "Playing alarm sound:" << alarmSound->source().toString();
        }
    } else {
        if (alarmSound->isPlaying()) {
            alarmSound->stop();
            qInfo() << "Stopping alarm sound.";
        }
    }
}


void AlarmGui::armSystem()
{
     qInfo() << "GUI: Arm button clicked.";
    if (alarmController) {
        alarmController->arm(); // Directly call the method
    }
}

void AlarmGui::disarmSystem()
{
     qInfo() << "GUI: Disarm button clicked.";
    if (alarmController) {
        alarmController->disarm(); // Directly call the method
    }
}

void AlarmGui::resetSystem()
{
    qInfo() << "GUI: Reset button clicked.";
    if (alarmController) {
        alarmController->resetTrigger(); // Directly call the method
    }
}

void AlarmGui::updateButtonStates(AlarmState currentState)
{
    if (!backendInitialized) return; // Don't update if backend failed

    armButton->setEnabled(currentState == AlarmState::DISARMED);
    disarmButton->setEnabled(currentState == AlarmState::ARMED || currentState == AlarmState::TRIGGERED);
    resetButton->setEnabled(currentState == AlarmState::TRIGGERED);
}

void AlarmGui::closeEvent(QCloseEvent *event)
{
    qInfo() << "Close event received. Cleaning up backend...";
    cleanupBackend(); // Ensure threads are stopped on close
    QMainWindow::closeEvent(event); // Call base class implementation
}