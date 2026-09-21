/**
 * @file main.cpp
 * @brief Primary daemon execution boundary.
 * @details Establishes the real-time event loop maintaining deterministic execution bounds
 * under 33ms to comply with the defined Zero-PII privacy processing constraints.
 */

#include "Config.hpp"
#include "Watchdog.hpp"
#include "AnalyticsCore.hpp"
#include "MqttManager.hpp"
#include "InferencePipeline.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

// Linux POSIX headers required for RTC manipulation
#include <sys/time.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

std::atomic<bool> g_running{true};

// Handle process termination cleanly
void signal_handler(int) { 
    g_running = false; 
}

// Modifies system time and flashes the physical DS3231 module via /dev/rtc0
void synchronize_ds3231_rtc(long long epoch_sec) {
    // 1. Update the volatile Linux OS time
    struct timeval tv;
    tv.tv_sec = epoch_sec;
    tv.tv_usec = 0;
    if (settimeofday(&tv, nullptr) < 0) {
        std::cerr << "[RTC] Failed to set OS system time. (Requires sudo/root)\n";
        return;
    }

    // 2. Push time to the DS3231 hardware module
    int rtc_fd = open("/dev/rtc0", O_RDWR);
    if (rtc_fd >= 0) {
        struct tm* tm_info = gmtime(&tv.tv_sec);
        struct rtc_time rt = {0};
        
        rt.tm_sec = tm_info->tm_sec;
        rt.tm_min = tm_info->tm_min;
        rt.tm_hour = tm_info->tm_hour;
        rt.tm_mday = tm_info->tm_mday;
        rt.tm_mon = tm_info->tm_mon;
        rt.tm_year = tm_info->tm_year;
        
        if (ioctl(rtc_fd, RTC_SET_TIME, &rt) < 0) {
            std::cerr << "[RTC] I2C ioctl failed to write to DS3231.\n";
        } else {
            std::cout << "[RTC] System and DS3231 successfully synchronized to epoch: " << epoch_sec << "\n";
        }
        close(rtc_fd);
    } else {
        std::cerr << "[RTC] Could not open /dev/rtc0 hardware node.\n";
    }
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Initialize the hardware abstraction layer
    REN::HardwareWatchdog watchdog(REN::Config::GPIO_CHIP, REN::Config::WATCHDOG_PIN);
    REN::MqttManager mqtt_manager(REN::Config::MQTT_BROKER_URI, REN::Config::MQTT_CLIENT_ID);
    
    // Instantiate Analytics with a closure routing surge alerts to the MQTT stack
    REN::AnalyticsCore analytics([&mqtt_manager](const std::string& surge_payload) {
        mqtt_manager.publish_realtime_surge(surge_payload);
    });

    // Map the external sync request directly to the internal data extraction pipeline
    mqtt_manager.set_sync_request_handler([&analytics]() {
        return analytics.extract_and_flush_data();
    });

    // Register the RTC sync handler
    mqtt_manager.set_activation_handler([](long long timestamp) {
        synchronize_ds3231_rtc(timestamp);
    });

    mqtt_manager.connect();
    
    REN::InferencePipeline pipeline;
    if (!pipeline.initialize_camera()) {
        std::cerr << "[Main] Abort: Camera initialization failed.\n";
        return -1;
    }

    // Deterministic Application Loop
    while (g_running) {
        auto start_time = std::chrono::steady_clock::now();

        // 1. Data Ingestion & Tracking
        std::vector<REN::TrackedObject> tracklets = pipeline.step();
        
        // 2. Queue Physics and Homography Translation
        analytics.process_frame(tracklets);

        // 3. Service Hardware Coprocessor
        watchdog.kick();

        // 4. Budget Enforcement
        // Governs CPU timing strictly to the 33ms execution cap required for Zero-PII[cite: 1]
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
        if (elapsed.count() < 33) {
            std::this_thread::sleep_for(std::chrono::milliseconds(33 - elapsed.count()));
        }
    }
    return 0;
}