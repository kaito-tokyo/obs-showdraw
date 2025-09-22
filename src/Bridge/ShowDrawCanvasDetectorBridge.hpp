#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace ShowDraw {

struct CanvasDetectorResult {
    float confidence;
    float x;
    float y;
    float width;
    float height;
};

class ShowDrawCanvasDetectorBridge {
public:
    ShowDrawCanvasDetectorBridge();
    ~ShowDrawCanvasDetectorBridge();

    ShowDrawCanvasDetectorBridge(const ShowDrawCanvasDetectorBridge&) = delete;
    ShowDrawCanvasDetectorBridge& operator=(const ShowDrawCanvasDetectorBridge&) = delete;
    ShowDrawCanvasDetectorBridge(ShowDrawCanvasDetectorBridge&&) noexcept;
    ShowDrawCanvasDetectorBridge& operator=(ShowDrawCanvasDetectorBridge&&) noexcept;

    std::vector<CanvasDetectorResult> detect(const uint8_t* bgra_data, int width, int height);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

}
