# Proximity alarm system

Our project is a simple alarm system designed for embedded Linux platforms like Raspberry Pi. It utilizes PIR (Passive Infrared) and VCNL4010 proximity sensors to detect intrusions, controls an alarm sound, and provides status updates and control via a web API. An optional Qt-based graphical user interface (GUI) is included but is currently **incomplete and considered experimental (TODO)**.

## Features

* Monitors GPIO for PIR sensor events.
* Monitors I2C for VCNL4010 proximity sensor readings.
* Manages alarm states: `DISARMED`, `ARMED`, `TRIGGERED`.
* Triggers an audible alarm (requires external sound player).
* Provides a RESTful API server (built with cpp-httplib) for status and control.
* Includes a basic web frontend (HTML/JS/TailwindCSS) to interact with the API.
* **Experimental**: Optional Qt5-based GUI for local control and status monitoring (Currently incomplete).

## Dependencies

### Core Dependencies (Required for both API Server and GUI)

* **CMake**: Version 3.20 or higher ([cmake_minimum_required(VERSION 3.20.0)](/src/CMakeLists.txt?line=1)).
* **C++ Compiler**: Supporting C++20 ([set(CMAKE_CXX_STANDARD 20)](/src/CMakeLists.txt?line=4)).
* **pkg-config**: Used by CMake to find libraries.
* **libgpiod**: Version 1.6 ([pkg_check_modules(GPIOD REQUIRED libgpiod>=1.6)](/src/CMakeLists.txt?line=9)). Required for GPIO interaction.
* **C++ Standard Threads**: The project uses `<thread>`, `<mutex>`, etc. from the C++ standard library. The CMake check for `Threads::Threads` ([find_package(Threads REQUIRED)](/src/CMakeLists.txt?line=10)) ensures the underlying platform support (like pthreads) needed by the standard library is available.

### API Server Dependencies (`RTEP` target)

* **cpp-httplib**: Header-only HTTP/HTTPS library (included in `src/third_party/cpp-httplib/httplib.h`).
* **nlohmann/json**: JSON library for C++ (downloaded via FetchContent during CMake configuration).

### GUI Dependencies (`RTEP_GUI` target - Optional, Experimental)

* **Qt5**: Version 5.x is required if building the GUI ([find_package(Qt5 REQUIRED...)](/src/CMakeLists.txt?line=48)). Specifically needs the following components:
    * `Core`
    * `Gui`
    * `Widgets`
    * `Multimedia` ([target_link_libraries(RTEP_GUI PRIVATE Qt5::Core... Qt5::Multimedia)](/src/src/gui/alarmgui.cpp?line=1))
    ***Note**: The GUI implementation is currently **incomplete/TODO**.

### Runtime Dependencies


* **I2C Support**: The kernel must support I2C, and potentially `libi2c-dev` (or equivalent kernel headers) might be needed depending on the system for I2C communication via ioctl ([I2cHandler.cpp](/src/src/I2cHandler.cpp)). **Crucially, the I2C interface on the target device (e.g., Raspberry Pi) may need to be explicitly enabled (e.g., using `raspi-config` or device tree overlays).**
* **(Optional) Qt5 runtime libraries**: Required on the target system if running the GUI version.
* **Sound Player**: A command-line sound player is needed to play the alarm sound for the `RTEP` target. The code currently uses `mpv` ([SOUND_PLAYER_CMD = "mpv --loop=inf"](/src/src/main.cpp?line=33)) but mentions `aplay` or `mpg123` as alternatives ([AlarmController.cpp](/src/src/AlarmController.cpp?line=33)). The GUI uses QtMultimedia.
* **Process Killer**: A command to stop the sound player for the `RTEP` target. The code uses `pkill` ([SOUND_STOP_CMD = "pkill mpv"](/src/src/main.cpp?line=34)).

## Building

### Prerequisites

1.  Ensure all **Core Dependencies** are installed on your build system. On Debian/Ubuntu-based systems:
    ```bash
    sudo apt update
    sudo apt install build-essential cmake pkg-config libgpiod-dev libi2c-dev
    ```
2.  **Enable I2C**: On Raspberry Pi, ensure I2C is enabled using `sudo raspi-config` -> Interface Options -> I2C. Reboot if required.
3.  **(Optional)** If building the GUI, install Qt5 development packages:
    ```bash
    # Example for Debian/Ubuntu - package names might vary
    sudo apt install qtbase5-dev qtmultimedia5-dev
    ```

### Build Steps

1.  **Clone/Prepare the Project**: Make sure you have the project code.
2.  **Create Build Directory**:
    ```bash
    cd RTEP-Project
    mkdir build
    cd build
    ```
3.  **Configure with CMake**:
    * **To build only the API server (default):**
        ```bash
        cmake ../src
        ```
    * **To build the API server AND the experimental Qt GUI:** ([BUILD_GUI option](file:///RTEP-Project/src/CMakeLists.txt?line=45))
        ```bash
        cmake ../src -DBUILD_GUI=ON
        ```
4.  **Compile**:
    ```bash
    make -j$(nproc) # Use multiple cores if available
    ```
5.  **Executables**: The compiled executables will be in the `build` directory:
    * `RTEP` (API Server)
    * `RTEP_GUI` (Qt GUI, if built)

## Configuration

Several parameters can be configured directly in the source code before compiling:

* **GPIO Chip/Line**: In `src/main.cpp` ([GPIO_CHIP](/src/src/main.cpp?line=18), [PIR_GPIO_LINE](/src/src/main.cpp?line=19)) or `src/gui/alarmgui.h` ([GPIO_CHIP](/src/src/gui/alarmgui.h?line=35), [PIR_GPIO_LINE](/src/src/gui/alarmgui.h?line=36)) for the GUI.
* **I2C Device/Address**: In `src/main.cpp` ([I2C_DEVICE](src/src/main.cpp?line=20), [VCNL4010_ADDR](/src/src/main.cpp?line=21)) or `src/gui/alarmgui.h` ([I2C_DEVICE]/src/src/gui/alarmgui.h?line=37), [VCNL4010_ADDR](f/src/src/gui/alarmgui.h?line=38)).
* **I2C Polling/Threshold**: In `src/main.cpp` ([I2C_POLL_INTERVAL_MS](/src/src/main.cpp?line=22), [PROXIMITY_THRESHOLD](/src/src/main.cpp?line=23)) or `src/gui/alarmgui.h` ([I2C_POLL_INTERVAL_MS](/src/src/gui/alarmgui.h?line=39), [PROXIMITY_THRESHOLD](/src/src/gui/alarmgui.h?line=40)).
* **API Host/Port**: In `src/main.cpp` ([API_HOST](/src/src/main.cpp?line=24), [API_PORT](/src/src/main.cpp?line=25)).
* **Alarm Sound**: File path and play/stop commands for the `RTEP` target in `src/main.cpp` ([ALARM_SOUND_FILE](/src/src/main.cpp?line=32), [SOUND_PLAYER_CMD](/src/src/main.cpp?line=33), [SOUND_STOP_CMD](/src/src/main.cpp?line=34)). The GUI version ([`src/gui/alarmgui.h`](/src/src/gui/alarmgui.h?line=41)) uses QtMultimedia internally for sound playback, only needing the file path.

## Running

### API Server (`RTEP`)

1.  Ensure the executable has permissions: `chmod +x RTEP`
2.  Run the server (requires appropriate permissions for GPIO/I2C, often root or membership in specific groups like `gpio`, `i2c`):
    ```bash
    cd RTEP-Project/build
    sudo ./RTEP # Or run without sudo if permissions allow
    ```
3.  The API server will listen on the configured host and port (default: `0.0.0.0:8080`). Check console output for confirmation or errors.

### Qt GUI (`RTEP_GUI` - Experimental)

***Note**: This GUI is currently incomplete and intended for development/testing.*
1.  Ensure the GUI was built (`-DBUILD_GUI=ON`).
2.  Ensure the executable has permissions: `chmod +x RTEP_GUI`
3.  Run the GUI (requires appropriate permissions for GPIO/I2C and potentially access to the display server):
    ```bash
    cd RTEP-Project/build
    ./RTEP_GUI
    ```
    *Note: Running GUI applications with `sudo` can sometimes cause issues with display server connections. Ensure the user running the GUI has the necessary hardware permissions.*

### Web Frontend (`web/frontend.html`)

1.  The web frontend (`web/frontend.html`) needs the `RTEP` API server to be running and accessible from the browser.
2.  **Local Access**: Open the `frontend.html` file directly in a web browser *on the same machine* where the API server is running. The API calls (e.g., `Workspace('/status')`) will target `localhost` (or the relevant local IP) at the default port (80).
3.  **Remote Access**: To control the system from a different device (e.g., phone or another computer):
    * You **must** deploy the `frontend.html` file (and potentially the entire `web` directory if it includes CSS/JS files in the future) onto an **HTTP server** (like Nginx, Apache, or a simple Python server).
    * The browser accessing the frontend needs network access to the machine running the `RTEP` API server.
    * If accessing from outside your local network, you will likely need to configure **port forwarding** on your router to forward external requests (e.g., on port 80 or 8080) to the internal IP address and port of the device running the `RTEP` API server.
    * Ensure the API URLs within `frontend.html` correctly point to the accessible IP/domain and port of the API server. Currently, they use relative paths (`/status`), which assumes the frontend is served from the same origin or via a correctly configured reverse proxy.

## Deployment

1.  **Copy Executable(s)**: Copy the required executable (`RTEP` and/or `RTEP_GUI`) from the `build` directory to the target device.
2.  **Copy Sound File**: Copy the `alarm.wav` (or your chosen sound file) to the location expected by the application (e.g., next to the executable, see `ALARM_SOUND_FILE` configuration).
3.  **Install Dependencies**: Ensure all **Runtime Dependencies** are installed on the target device.
4.  **(Optional) Copy Web Frontend**: If using the web interface remotely, deploy the `web` directory contents to a suitable web server on the target device or another accessible machine.
5.  **Enable Hardware**: Ensure I2C is enabled on the target device (e.g., via `raspi-config`).
6.  **Permissions**: Ensure the user running the application has permissions to access `/dev/gpiomem` (or the specific chip device), `/dev/i2c-*`, and execute the sound player/killer commands. This often involves adding the user to `gpio` and `i2c` groups: `sudo usermod -aG gpio,i2c <username>`. A reboot or logout/login might be needed for group changes to take effect.
7.  **Run**: Execute the application as described in the **Running** section. Consider running it as a system service (e.g., using systemd) for robustness.

## API Endpoints

The `RTEP` server provides the following endpoints:

* `GET /status`: Retrieves the current alarm state, last trigger source, and sensor status.
    * Response: `application/json`
        ```json
        {
          "state": "ARMED" | "DISARMED" | "TRIGGERED",
          "last_trigger": "None" | "PIR" | "PROXIMITY",
          "sensors": {
            "pir_active": true | false,
            "proximity_active": true | false
          }
        }
        ```
* `POST /arm`: Arms the system.
    * Response: `application/json`
        ```json
        {
          "status": "success",
          "message": "System armed.",
          "current_state": "ARMED"
        }
        ```
* `POST /disarm`: Disarms the system.
    * Response: `application/json`
        ```json
        {
          "status": "success",
          "message": "System disarmed.",
          "current_state": "DISARMED"
        }
        ```
* `POST /reset`: Resets the system from the `TRIGGERED` state back to `ARMED`.
    * Response: `application/json`
        ```json
        {
          "status": "success",
          "message": "Alarm trigger reset.",
          "current_state": "ARMED"
        }
        ```

## Social Media Account

https://youtube.com/@team13-i8t?si=IQBlaMENYFneyDBU

## Response Delay
During development, we roughly tested the delay using logs. The delay from the sensor signal emission to the triggering of the alarm handler is relatively low, sometimes less than 1 millisecond. However, the sensor itself has a triggering delay, and the VCNL4010 sensor we used internally collects and reports data periodically. Based on our configuration, it reports data every 80 milliseconds. Additionally, our web front-end page collects current status through polling a RESTful API every 200 milliseconds, which may lead to a delay of around 300 milliseconds in extreme cases. Furthermore, since we utilize an external player to play alarm sound effects, this may introduce tens to hundreds of milliseconds of delay depending on system scheduling. Nonetheless, in the worst-case scenario, our response delay will not exceed one second.

## License

GPLv3