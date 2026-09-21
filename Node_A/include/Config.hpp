/**
 * @file Config.hpp
 * @brief Global configuration parameters for Node A Vision Core.
 * @details Defines spatial limits, hardware pins, mathematical constants for queue physics,
 * and MQTT network topology. Inline variables enforce One Definition Rule (ODR) across units.
 */

#pragma once
#include <Eigen/Dense>
#include <string>
#include <vector>
#include <opencv2/core.hpp>

// Add to MQTT Broker & Topics section
inline constexpr const char* TOPIC_NODE_ACTIVATE = "store/commands/activate";

namespace REN::Config {
    // Spatial Limits & Floor Grid Configuration
    inline constexpr int FRAME_WIDTH = 1920;
    inline constexpr int FRAME_HEIGHT = 1080;
    
    // Homography heatmap dimensions representing the floor in 10cm x 10cm physical bins[cite: 1]
    inline constexpr int CAD_GRID_WIDTH = 100;
    inline constexpr int CAD_GRID_HEIGHT = 100;
    
    // Module 1: Directional Tripwire Vectors
    // Defines the physical threshold A-B for entrance/exit footfall tracking[cite: 1]
    inline const cv::Point2f TRIPWIRE_A(100.0f, 900.0f);
    inline const cv::Point2f TRIPWIRE_B(1800.0f, 900.0f);
    
    // Module 1: Promotional Micro-Zone Boundaries
    // Geometric bounds for detecting customer engagement at endcap displays[cite: 1]
    inline const std::vector<cv::Point2f> PROMO_ZONE = {
        {500.0f, 500.0f}, {800.0f, 500.0f}, {800.0f, 800.0f}, {500.0f, 800.0f}
    };
    
    // Engagement threshold parameters: requires sustained presence and low velocity[cite: 1]
    inline constexpr double DWELL_TIME_MIN = 5.0; // Minimum seconds to qualify as dwell[cite: 1]
    inline constexpr double VELOCITY_MAX = 0.2;   // Maximum m/s to filter transit vs browsing[cite: 1]

    // Module 3: Queue Intelligence & Surge Physics
    // Vector pointing towards the physical checkout area[cite: 1]
    inline const Eigen::Vector2f U_CHECKOUT(0.0f, -1.0f); 
    
    // Cashier capacity (customers processed per minute) and baseline staffing[cite: 1]
    inline constexpr double MU_SERVICE = 2.5; 
    inline constexpr int C_ACTIVE = 2; 

    // Hardware Watchdog Integration (TI TPS3823 Daemon)[cite: 1]
    // Specifies the GPIO controller and pin bound to the supervisor IC
    inline constexpr const char* GPIO_CHIP = "gpiochip1";
    inline constexpr unsigned int WATCHDOG_PIN = 17;

    // MQTT Topology Configuration
    // Payload sizing restricted to ~15 KB/s continuous bandwidth[cite: 1]
    inline constexpr const char* MQTT_BROKER_URI = "tcp://192.168.1.100:1883";
    inline constexpr const char* MQTT_CLIENT_ID = "REN_NodeA_VisionCore";
    inline constexpr const char* TOPIC_SURGE_ALERT = "store/telemetry/events";
    inline constexpr const char* TOPIC_SYNC_REQUEST = "store/requests/sync";
    inline constexpr const char* TOPIC_SYNC_RESPONSE = "store/data/sync";

    // Non-Volatile Persistence Configuration
    // Targets a dedicated Read-Write partition to protect end-of-day KPIs from unexpected power loss
    inline constexpr const char* KPI_STORAGE_PATH = "/data/local_kpi_store.json";
    inline constexpr int DISK_FLUSH_INTERVAL_SEC = 5;
}