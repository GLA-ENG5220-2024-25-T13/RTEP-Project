cmake_minimum_required(VERSION 3.20.0)
project(RTEP VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Dependencies ---

# libgpiod
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod>=1.6) # Ensure version 1.6.x is found

# pthreads (standard on Linux)
find_package(Threads REQUIRED)

# glog
# find_package(glog REQUIRED)

# cpp-httplib (header only - provide path)
include_directories(third_party/cpp-httplib)

# nlohmann/json
include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# --- Source Files ---
set(SOURCES
    src/main.cpp
    src/AlarmController.cpp
    src/GpioHandler.cpp
    src/I2cHandler.cpp
    src/ApiServer.cpp
)

set(HEADERS
    src/AlarmController.h
    src/GpioHandler.h
    src/I2cHandler.h
    src/ApiServer.h
)


# --- Executable ---
add_executable(RTEP ${SOURCES} ${HEADERS})

# --- Linking ---
target_include_directories(RTEP PUBLIC
    # glog::glog
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GPIOD_INCLUDE_DIRS} # From pkg-config
)

target_link_libraries(RTEP PRIVATE
    # glog::glog
    ${GPIOD_LINK_LIBRARIES}      # From pkg-config (-lgpiod)
    Threads::Threads        # C++11 threads require linking pthread
    nlohmann_json::nlohmann_json
)

# --- Installation ---
# install(TARGETS RTEP DESTINATION bin)

# Print info
message(STATUS "Using libgpiod include dirs: ${GPIOD_INCLUDE_DIRS}")
message(STATUS "Using libgpiod libraries: ${GPIOD_LIBRARIES}")