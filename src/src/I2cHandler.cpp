#include "I2cHandler.h"
#include <iostream>
#include <chrono>
#include <unistd.h> // For open, close, read, write
#include <fcntl.h>  // For O_RDWR
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h> // May require installing kernel headers or using libi2c-dev
#include <system_error>
#include <cstring> // For strerror

// VCNL4010 Register Addresses (From Datasheet)
#define VCNL4010_I2C_ADDR 0x13 // Default address
#define VCNL4010_REG_COMMAND 0x80
#define VCNL4010_REG_PRODUCT_ID 0x81
#define VCNL4010_REG_PROX_RATE 0x82
#define VCNL4010_REG_PROX_CURRENT 0x83
#define VCNL4010_REG_AMBIENT_LIGHT 0x85
#define VCNL4010_REG_PROX_DATA_MSB 0x87
#define VCNL4010_REG_PROX_DATA_LSB 0x88
#define VCNL4010_REG_INT_CONTROL 0x89

// VCNL4010 Command Bits
#define VCNL4010_CMD_SELFTIMED_ENABLE 0x07 // Use self-timed mode

// VCNL4010 Configuration values (Example)
#define VCNL4010_PROX_RATE_HZ 3     // ~3.9 Hz (check datasheet for values 0-7)
#define VCNL4010_PROX_CURRENT_MA 20 // 200mA LED current (check datasheet, 0-20 -> 0-200mA)

// Helper for I2C communication (using smbus functions if available, otherwise raw ioctl)

// Note: Using i2c_smbus_* functions requires linking with -li2c and libi2c-dev
// Using raw ioctl avoids the extra library dependency. We'll use ioctl here.

// Simple ioctl based read/write byte data
static inline int i2c_write_byte_data(int fd, uint8_t reg, uint8_t value)
{
    uint8_t outbuf[2] = {reg, value};
    struct i2c_msg msg = {
        .addr = VCNL4010_I2C_ADDR, // Assuming address is already set by ioctl(I2C_SLAVE)
        .flags = 0,                // Write
        .len = 2,
        .buf = outbuf};
    struct i2c_rdwr_ioctl_data msgset = {
        .msgs = &msg,
        .nmsgs = 1};
    if (ioctl(fd, I2C_RDWR, &msgset) < 0)
    {
        perror("ioctl(I2C_RDWR) write failed");
        return -1;
    }
    return 0;
}

// Simple ioctl based read word data (2 bytes)
static inline int i2c_read_word_data(int fd, uint8_t reg, uint16_t *value)
{
    uint8_t outbuf[1] = {reg};
    uint8_t inbuf[2] = {0};
    struct i2c_msg msgs[2] = {
        {// First message: write register address
         .addr = VCNL4010_I2C_ADDR,
         .flags = 0, // Write
         .len = 1,
         .buf = outbuf},
        {// Second message: read data
         .addr = VCNL4010_I2C_ADDR,
         .flags = I2C_M_RD, // Read
         .len = 2,
         .buf = inbuf}};
    struct i2c_rdwr_ioctl_data msgset = {
        .msgs = msgs,
        .nmsgs = 2};

    if (ioctl(fd, I2C_RDWR, &msgset) < 0)
    {
        perror("ioctl(I2C_RDWR) read failed");
        return -1;
    }
    // VCNL4010 data is MSB first
    *value = ((uint16_t)inbuf[0] << 8) | inbuf[1];
    return 0;
}

// Simple ioctl based read byte data
static inline int i2c_read_byte_data(int fd, uint8_t reg, uint8_t *value)
{
    uint8_t outbuf[1] = {reg};
    uint8_t inbuf[1] = {0};
    struct i2c_msg msgs[2] = {
        {// First message: write register address
         .addr = VCNL4010_I2C_ADDR,
         .flags = 0, // Write
         .len = 1,
         .buf = outbuf},
        {// Second message: read data
         .addr = VCNL4010_I2C_ADDR,
         .flags = I2C_M_RD, // Read
         .len = 1,
         .buf = inbuf}};
    struct i2c_rdwr_ioctl_data msgset = {
        .msgs = msgs,
        .nmsgs = 2};

    if (ioctl(fd, I2C_RDWR, &msgset) < 0)
    {
        perror("ioctl(I2C_RDWR) read failed");
        return -1;
    }
    // VCNL4010 data is MSB first
    *value = inbuf[0];
    return 0;
}

I2cHandler::I2cHandler(AlarmController &controller, const std::string &devicePath, uint8_t deviceAddr)
    : alarmController(controller), i2cDevicePath(devicePath), i2cDeviceAddr(deviceAddr),
      running(false), pollingIntervalMs(200), proximityThreshold(3000) {} // Default values

I2cHandler::~I2cHandler()
{
    stopMonitoring();
    if (fd >= 0)
    {
        close(fd);
    }
}

bool I2cHandler::initialize()
{
    fd = open(i2cDevicePath.c_str(), O_RDWR);
    if (fd < 0)
    {
        std::cerr << "ERROR: Failed to open I2C device '" << i2cDevicePath << "': " << strerror(errno) << std::endl;
        return false;
    }

    if (ioctl(fd, I2C_SLAVE, i2cDeviceAddr) < 0)
    {
        std::cerr << "ERROR: Failed to set I2C slave address " << std::hex << (int)i2cDeviceAddr << ": " << strerror(errno) << std::endl;
        close(fd);
        fd = -1;
        return false;
    }
    std::cout << "Opened I2C device " << i2cDevicePath << " for address 0x" << std::hex << (int)i2cDeviceAddr << std::dec << std::endl;

    if (!configureSensor())
    {
        close(fd);
        fd = -1;
        return false;
    }

    return true;
}

bool I2cHandler::configureSensor()
{
    uint8_t PRODUCT_ID = 0;
    std::cout << "Configuring VCNL4010..." << std::endl;
    // Check product id
    i2c_read_byte_data(fd, VCNL4010_REG_PRODUCT_ID, &PRODUCT_ID);
    if (PRODUCT_ID != 0x21)
        return false
    // Set proximity rate (3.9 Hz)
    if (i2c_write_byte_data(fd, VCNL4010_REG_PROX_RATE, VCNL4010_PROX_RATE_HZ) < 0)
        return false;
    // Set LED current (200mA)
    if (i2c_write_byte_data(fd, VCNL4010_REG_PROX_CURRENT, VCNL4010_PROX_CURRENT_MA) < 0)
        return false;
    // Enable self-timed proximity measurements
    if (i2c_write_byte_data(fd, VCNL4010_REG_COMMAND, VCNL4010_CMD_SELFTIMED_ENABLE) < 0)
        return false;

    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Short delay after config
    std::cout << "VCNL4010 configuration done." << std::endl;
    return true;
}

bool I2cHandler::readProximity(uint16_t &value)
{
    // VCNL4010 in self-timed mode updates data automatically. We just read it.
    // If using on-demand mode, you'd write to COMMAND register first.
    if (i2c_read_word_data(fd, VCNL4010_REG_PROX_DATA_MSB, &value) < 0)
    {
        std::cerr << "ERROR: Failed to read proximity data." << std::endl;
        return false;
    }
    return true;
}

void I2cHandler::startMonitoring(int intervalMs, uint16_t threshold)
{
    if (fd < 0)
    {
        std::cerr << "ERROR: I2C device not initialized. Cannot start monitoring." << std::endl;
        return;
    }
    if (running.load())
    {
        std::cout << "I2C monitor already running." << std::endl;
        return;
    }

    pollingIntervalMs = intervalMs;
    proximityThreshold = threshold;
    running.store(true);
    monitorThread = std::thread(&I2cHandler::monitorLoop, this);
    std::cout << "I2C monitoring thread started (Interval: " << pollingIntervalMs << "ms, Threshold: " << proximityThreshold << ")" << std::endl;
}

void I2cHandler::stopMonitoring()
{
    if (running.exchange(false))
    { // Atomically set running to false and check previous value
        std::cout << "Stopping I2C monitoring thread..." << std::endl;
        if (monitorThread.joinable())
        {
            monitorThread.join();
            std::cout << "I2C monitoring thread stopped." << std::endl;
        }
    }
}

void I2cHandler::monitorLoop()
{
    uint16_t proxValue;
    while (running.load())
    {
        if (readProximity(proxValue))
        {
            std::cout << "Proximity: " << proxValue << std::endl; // Debugging output

            // Check threshold only if armed
            if (alarmController.isArmed() && proxValue > proximityThreshold)
            {
                std::cout << "Proximity threshold exceeded (" << proxValue << " > " << proximityThreshold << ")" << std::endl;
                alarmController.trigger("PROXIMITY");
                // TODO: Add debounce logic here
            }
        }
        else
        {
            // Handle read error (e.g., log, maybe try to re-init)
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Avoid busy loop on error
        }

        // Wait for the next polling interval
        std::this_thread::sleep_for(std::chrono::milliseconds(pollingIntervalMs));
    }
    std::cout << "I2C monitor loop finished." << std::endl;
}