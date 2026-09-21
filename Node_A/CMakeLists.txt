# CMake configuration for REN_NodeA_VisionCore
# Handles build requirements for the real-time embedded computer vision pipeline.

cmake_minimum_required(VERSION 3.16)
project(Node_A LANGUAGES CXX)

# Enforce C++17 standard for modern language features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimize for execution speed (-O3) and enable comprehensive warnings (-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall")

# Core AI and Vision Dependencies
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED NO_MODULE)
find_package(Threads REQUIRED)

# Telemetry and JSON Dependencies
find_package(nlohmann_json REQUIRED)

# Hardware Control and Networking Dependencies
find_library(PAHO_MQTT_CPP paho-mqttpp3 REQUIRED)
find_library(PAHO_MQTT_C paho-mqtt3as REQUIRED)
find_library(GPIOD gpiod REQUIRED)

# --- 1. Define all source and header files explicitly ---
# Adding headers here forces the IDE to apply include paths to them
add_executable(node_a_core
    src/main.cpp
    src/Watchdog.cpp
    src/AnalyticsCore.cpp
    src/MqttManager.cpp
    src/InferencePipeline.cpp
    
    # Add your header files here so the IDE recognizes them
    include/Watchdog.hpp
    include/AnalyticsCore.hpp
    include/MqttManager.hpp
    include/InferencePipeline.hpp
)

# --- 2. Use target-specific include directories (Modern CMake) ---
target_include_directories(node_a_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

# --- 3. Link dependencies ---
target_link_libraries(node_a_core
    PRIVATE
    ${OpenCV_LIBS}
    Eigen3::Eigen                  # This automatically handles Eigen include paths transitively
    nlohmann_json::nlohmann_json
    ${PAHO_MQTT_CPP}
    ${PAHO_MQTT_C}
    ${GPIOD}
    Threads::Threads
)
