cmake_minimum_required(VERSION 3.20.0)
project(RTEP VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        e88cb95b92acbdce9b058dd894a68e1281b38495
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(PkgConfig)
find_package(glog REQUIRED)
pkg_check_modules(GPIO libgpiodcxx REQUIRED)
include_directories(glog::glog ${GPIO_INCLUDE_DIRS})
add_executable(RTEP main.cpp)
target_link_libraries(RTEP glog::glog ${GPIO_LINK_LIBRARIES})