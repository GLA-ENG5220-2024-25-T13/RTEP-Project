cmake_minimum_required(VERSION 3.20.0)
project(RTEP_Monorepo VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Find Common Dependencies (Needed by both targets potentially) ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod>=1.6)
find_package(Threads REQUIRED)
message(STATUS "Found common dependencies: PkgConfig, GPIOD (>=1.6), Threads")
message(STATUS "  libgpiod includes: ${GPIOD_INCLUDE_DIRS}")
message(STATUS "  libgpiod libraries: ${GPIOD_LIBRARIES}")


# --- Core Logic Source Files (Shared) ---
set(CORE_SOURCES
    AlarmController.cpp # Will be compiled differently based on macro!
    GpioHandler.cpp
    I2cHandler.cpp
)
set(CORE_HEADERS
    AlarmController.h
    GpioHandler.h
    I2cHandler.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "Defining Original Executable target 'RTEP'")
include_directories(third_party/cpp-httplib) 
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)
message(STATUS "Found API dependencies: httplib (header), nlohmann::json (FetchContent)")

set(RTEP_APP_SOURCES
    main.cpp      # Original main entry point
    ApiServer.cpp # API Server code
    ${CORE_SOURCES} # Compile core sources directly for this target
)
set(RTEP_APP_HEADERS
    ApiServer.h
    ${CORE_HEADERS} # Include core headers
)

add_executable(RTEP ${RTEP_APP_SOURCES} ${RTEP_APP_HEADERS})
target_link_libraries(RTEP PRIVATE
    nlohmann_json::nlohmann_json # Link json for the API server
    ${GPIOD_LIBRARIES}           # Link common deps
    Threads::Threads
)
# Ensure RTEP target can find httplib headers relative to this file
target_include_directories(RTEP PRIVATE third_party/cpp-httplib)


# --- New Target: Optional Qt GUI Executable ---
option(BUILD_GUI "Build the optional Qt GUI application" OFF) # Default to OFF

if(BUILD_GUI)
    message(STATUS "Defining Qt GUI Executable target 'RTEP_GUI'")
    # Find Qt5
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia)
    message(STATUS "Found Qt5 components: Core, Gui, Widgets, Multimedia")

    # Enable AUTOMOC etc. for Qt
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON) 
    set(CMAKE_AUTORCC ON)
    set(CMAKE_INCLUDE_CURRENT_DIR ON) 

    set(GUI_SOURCES
        gui/main_gui.cpp    # Path relative to this CMakeLists.txt
        gui/alarmgui.cpp
        ${CORE_SOURCES}     # Compile core sources directly for this target too
    )
    set(GUI_HEADERS
        gui/alarmgui.h
        ${CORE_HEADERS}
    )

    add_executable(RTEP_GUI ${GUI_SOURCES} ${GUI_HEADERS})

    # *** Crucial Part: Define the macro ONLY for the GUI target ***
    target_compile_definitions(RTEP_GUI PRIVATE RTEP_BUILD_WITH_GUI)
    message(STATUS "  -> Defined RTEP_BUILD_WITH_GUI for RTEP_GUI target")

    target_link_libraries(RTEP_GUI PRIVATE
        # Link Qt Modules
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Multimedia
        # Link common deps
        ${GPIOD_LIBRARIES}
        Threads::Threads
    )
    # Ensure GUI target can find its own headers and core headers
    # Global include_directories(${CMAKE_CURRENT_SOURCE_DIR}) should cover core
    target_include_directories(RTEP_GUI PRIVATE gui) # Add gui subdir for its headers

else()
    message(STATUS "Optional GUI build is OFF (use -DBUILD_GUI=ON to enable)")
endif()