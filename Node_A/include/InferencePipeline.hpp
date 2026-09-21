/**
 * @file InferencePipeline.hpp
 * @brief V4L2 Hardware Video Engine integration layer.
 * @details Configures GStreamer DMA allocations for zero-copy memory operations, minimizing
 * CPU overhead by directing frame pipelines directly to LPDDR5 and Tensor cores.
 */

#pragma once
#include "AnalyticsCore.hpp"
#include <opencv2/videoio.hpp>
#include <string>
#include <vector>

namespace REN {

class InferencePipeline {
public:
    InferencePipeline();
    ~InferencePipeline();

    // Establishes physical camera binding
    bool initialize_camera();
    
    // Executes inference step, yielding tracklet coordinates and discarding the frame
    std::vector<TrackedObject> step();
    
private:
    cv::VideoCapture capture_;
    std::string get_dma_pipeline() const;
};

} // namespace REN