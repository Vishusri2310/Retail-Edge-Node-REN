/**
 * @file AnalyticsCore.hpp
 * @brief Primary mathematical engine for spatial dynamics and queue prediction.
 * @details Executes purely in volatile RAM. Analyzes 2D bounding boxes to extract bottom-center 
 * ground contact points, projects them onto a CAD homography plane, and derives arrival velocity vectors.
 */

#pragma once
#include "Config.hpp"
#include <opencv2/core.hpp>
#include <Eigen/Dense>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>
#include <functional>

namespace REN {

// Ephemeral tracking state maintained strictly in RAM for <33ms compliance[cite: 1]
struct ShopperState {
    std::chrono::time_point<std::chrono::steady_clock> first_seen;
    std::chrono::time_point<std::chrono::steady_clock> last_seen;
    Eigen::Vector2f first_pos;
    Eigen::Vector2f last_pos;
    bool in_promo_zone = false;
    double total_dwell = 0.0;
};

// Container for INT8 YOLO centroid extractions
struct TrackedObject {
    int id;
    cv::Rect2f bbox;
};

class AnalyticsCore {
public:
    using TelemetryCallback = std::function<void(const std::string&)>;

    explicit AnalyticsCore(TelemetryCallback cb);

    // Primary pipeline entry point per frame
    void process_frame(const std::vector<TrackedObject>& tracklets);
    
    // Yields JSON formatted metrics and immediately clears volatile arrays
    std::string extract_and_flush_data();
    
private:
    // Ray-casting geometric boundary validation
    bool point_in_polygon(const cv::Point2f& p, const std::vector<cv::Point2f>& poly);
    
    // Evaluates traversal across threshold A-B using vector cross products
    int evaluate_tripwire_cross_product(const Eigen::Vector2f& A, const Eigen::Vector2f& B, const Eigen::Vector2f& pt, const Eigen::Vector2f& last_pt);
    
    // File I/O for persisting end-of-day KPI resilience
    void load_state_from_disk();
    void save_state_to_disk();

    TelemetryCallback telemetry_callback_;
    mutable std::mutex core_mutex_;

    std::unordered_map<int, ShopperState> active_shoppers_;
    
    // Homography Matrix mapping pixels (x,y) to floor grid coordinates (X,Y)
    Eigen::Matrix3f homography_matrix_;
    
    // Aggregation Containers
    std::vector<int> completed_dwells_;
    int heatmap_[Config::CAD_GRID_HEIGHT][Config::CAD_GRID_WIDTH] = {{0}};
    int total_entries_ = 0;
    int total_exits_ = 0;

    std::chrono::time_point<std::chrono::steady_clock> last_disk_flush_;
};

} // namespace REN