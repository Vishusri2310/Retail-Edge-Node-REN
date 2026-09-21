#include "AnalyticsCore.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace REN {

AnalyticsCore::AnalyticsCore(TelemetryCallback cb) 
    : telemetry_callback_(std::move(cb)) {
    // Identity initialization for standard pixel coordinate scale
    homography_matrix_ << 1.0, 0.0, 0.0,
                          0.0, 1.0, 0.0,
                          0.0, 0.0, 1.0; 
    
    // Restore any un-synced metrics generated prior to power loss
    last_disk_flush_ = std::chrono::steady_clock::now();
    load_state_from_disk();
}

void AnalyticsCore::load_state_from_disk() {
    std::ifstream file(Config::KPI_STORAGE_PATH);
    if (!file.is_open()) return; // Clean initialization, no state exists

    try {
        nlohmann::json j;
        file >> j;
        
        total_entries_ = j.value("entries", 0);
        total_exits_ = j.value("exits", 0);
        
        if (j.contains("dwells")) {
            completed_dwells_ = j["dwells"].get<std::vector<int>>();
        }

        if (j.contains("heatmap")) {
            std::vector<int> flat = j["heatmap"].get<std::vector<int>>();
            int idx = 0;
            for (int r = 0; r < Config::CAD_GRID_HEIGHT; ++r) {
                for (int c = 0; c < Config::CAD_GRID_WIDTH; ++c) {
                    if (idx < flat.size()) {
                        heatmap_[r][c] = flat[idx++];
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[AnalyticsCore] Malformed disk state payload: " << e.what() << "\n";
    }
}

void AnalyticsCore::save_state_to_disk() {
    // Automatically create the parent directory if it doesn't exist
    std::filesystem::create_directories(std::filesystem::path(Config::KPI_STORAGE_PATH).parent_path());

    std::vector<int> flat_heatmap;
    flat_heatmap.reserve(Config::CAD_GRID_HEIGHT * Config::CAD_GRID_WIDTH);
    for (int r = 0; r < Config::CAD_GRID_HEIGHT; ++r) {
        for (int c = 0; c < Config::CAD_GRID_WIDTH; ++c) {
            flat_heatmap.push_back(heatmap_[r][c]);
        }
    }

    nlohmann::json j = {
        {"entries", total_entries_},
        {"exits", total_exits_},
        {"dwells", completed_dwells_},
        {"heatmap", flat_heatmap}
    };

    // Atomic write implementation to ensure filesystem integrity during power drops
    std::string temp_path = std::string(Config::KPI_STORAGE_PATH) + ".tmp";
    std::ofstream file(temp_path);
    if (file.is_open()) {
        file << j.dump();
        file.close();
        std::rename(temp_path.c_str(), Config::KPI_STORAGE_PATH);
    }
}

bool AnalyticsCore::point_in_polygon(const cv::Point2f& p, const std::vector<cv::Point2f>& poly) {
    bool is_inside = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        if (((poly[i].y > p.y) != (poly[j].y > p.y)) &&
            (p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x))
            is_inside = !is_inside;
    }
    return is_inside;
}

int AnalyticsCore::evaluate_tripwire_cross_product(const Eigen::Vector2f& A, const Eigen::Vector2f& B, const Eigen::Vector2f& pt, const Eigen::Vector2f& last_pt) {
    auto calc_cp = [&](const Eigen::Vector2f& c) {
        return (B.x() - A.x()) * (c.y() - A.y()) - (B.y() - A.y()) * (c.x() - A.x());
    };
    
    double cp_now = calc_cp(pt);
    double cp_last = calc_cp(last_pt);
    
    // Ignore exact edge hits to prevent floating point instability
    if (cp_now == 0.0 || cp_last == 0.0) return 0;
    
    // A sign flip indicates the vector crossed the threshold boundary
    if (std::signbit(cp_now) != std::signbit(cp_last)) {
        return (cp_now > 0.0) ? 1 : -1;
    }
    return 0;
}

void AnalyticsCore::process_frame(const std::vector<TrackedObject>& tracklets) {
    std::lock_guard<std::mutex> lock(core_mutex_);
    auto now = std::chrono::steady_clock::now();
    
    double lambda_predicted = 0.0;
    const double epsilon = 1e-6; // Prevents division by zero in kinematic projections
    bool kpi_updated = false;

    for (const auto& obj : tracklets) {
        // Isolate the ground contact point (bottom-center of bounding box)[cite: 1]
        cv::Point2f bottom_center(obj.bbox.x + obj.bbox.width * 0.5f, obj.bbox.y + obj.bbox.height);
        Eigen::Vector3f pixel_pt(bottom_center.x, bottom_center.y, 1.0f);
        
        // Execute Homography mapping to CAD floor plane
        Eigen::Vector3f floor_pt = homography_matrix_ * pixel_pt;
        floor_pt /= floor_pt.z(); 
        Eigen::Vector2f current_pos(floor_pt.x(), floor_pt.y());

        auto it = active_shoppers_.find(obj.id);
        if (it == active_shoppers_.end()) {
            ShopperState state;
            state.first_seen = state.last_seen = now;
            state.first_pos = state.last_pos = current_pos;
            active_shoppers_[obj.id] = state;
        } else {
            ShopperState& state = it->second;
            
            // Tripwire State Validation
            Eigen::Vector2f A(Config::TRIPWIRE_A.x, Config::TRIPWIRE_A.y);
            Eigen::Vector2f B(Config::TRIPWIRE_B.x, Config::TRIPWIRE_B.y);
            int dir = evaluate_tripwire_cross_product(A, B, current_pos, state.last_pos);
            if (dir == 1) { total_entries_++; kpi_updated = true; }
            else if (dir == -1) { total_exits_++; kpi_updated = true; }

            // Spatial Dwell and Velocity Validation
            double elapsed_time = std::max(std::chrono::duration_cast<std::chrono::milliseconds>(now - state.first_seen).count() / 1000.0, epsilon);
            double distance = (current_pos - state.first_pos).norm();
            double v_avg = distance / elapsed_time;

            bool inside_promo = point_in_polygon(bottom_center, Config::PROMO_ZONE);
            if (inside_promo && elapsed_time >= Config::DWELL_TIME_MIN && v_avg < Config::VELOCITY_MAX) {
                state.in_promo_zone = true;
                state.total_dwell = elapsed_time;
            }

            // Predictive Queue Physics
            // Projects the instantaneous velocity vector against the checkout vector field
            Eigen::Vector2f v_j = (current_pos - state.last_pos);
            Eigen::Vector2f d_j = current_pos;
            double norm_dj = std::max(d_j.norm(), epsilon); 
            double projection = v_j.dot(Config::U_CHECKOUT) / norm_dj;
            if (projection > 0.0) {
                lambda_predicted += projection;
            }

            state.last_seen = now;
            state.last_pos = current_pos;
        }

        // Discrete Heatmap Accumulation (10cm Bins)[cite: 1]
        int grid_x = static_cast<int>(floor_pt.x() * 10);
        int grid_y = static_cast<int>(floor_pt.y() * 10);
        if (grid_x >= 0 && grid_x < Config::CAD_GRID_WIDTH && grid_y >= 0 && grid_y < Config::CAD_GRID_HEIGHT) {
            heatmap_[grid_y][grid_x]++;
            kpi_updated = true;
        }
    }

    // Dynamic resource allocation trigger[cite: 1]
    if (lambda_predicted > (Config::MU_SERVICE * Config::C_ACTIVE)) {
        nlohmann::json surge_alert = {
            {"ts", std::chrono::system_clock::now().time_since_epoch().count()},
            {"zone", "checkout_buffer"},
            {"pred_surge", true},
            {"rec_counters", Config::C_ACTIVE + 1}
        };
        if (telemetry_callback_) telemetry_callback_(surge_alert.dump());
    }

    // Volatile RAM Cleanup
    for (auto it = active_shoppers_.begin(); it != active_shoppers_.end();) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen).count();
        if (elapsed > 2) {
            // Shopper definitively out of frame; archive active dwell time
            if (it->second.in_promo_zone) {
                completed_dwells_.push_back(static_cast<int>(it->second.total_dwell));
                kpi_updated = true;
            }
            it = active_shoppers_.erase(it);
        } else {
            ++it;
        }
    }

    // Scheduled Disk Persistence
    auto elapsed_since_flush = std::chrono::duration_cast<std::chrono::seconds>(now - last_disk_flush_).count();
    if (kpi_updated && elapsed_since_flush >= Config::DISK_FLUSH_INTERVAL_SEC) {
        save_state_to_disk();
        last_disk_flush_ = now;
    }
}

std::string AnalyticsCore::extract_and_flush_data() {
    std::lock_guard<std::mutex> lock(core_mutex_);
    
    // Flatten 2D array for standardized JSON transport
    std::vector<int> flat_heatmap;
    flat_heatmap.reserve(Config::CAD_GRID_HEIGHT * Config::CAD_GRID_WIDTH);
    for (int r = 0; r < Config::CAD_GRID_HEIGHT; ++r) {
        for (int c = 0; c < Config::CAD_GRID_WIDTH; ++c) {
            flat_heatmap.push_back(heatmap_[r][c]);
            heatmap_[r][c] = 0; // Scrub volatile structures
        }
    }

    nlohmann::json response = {
        {"node_id", "A1"},
        {"metrics", {
            {"entries", total_entries_},
            {"exits", total_exits_},
            {"dwells", completed_dwells_}
        }},
        {"heatmap", flat_heatmap}
    };

    total_entries_ = total_exits_ = 0;
    completed_dwells_.clear();
    
    // Ensure on-disk state is cleared concurrently with RAM extraction
    save_state_to_disk();
    
    return response.dump();
}

} // namespace REN