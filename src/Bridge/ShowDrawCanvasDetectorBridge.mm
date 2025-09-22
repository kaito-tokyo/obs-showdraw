#include "Bridge/ShowDrawCanvasDetectorBridge.hpp"

#import <CoreML/CoreML.h>
#import <Vision/Vision.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreGraphics/CoreGraphics.h>

#import <CoreMLShowDrawCanvasDetector/ShowDrawCanvasDetectorWrapper.h>
#import <CoreMLShowDrawCanvasDetector/ShowDrawCanvasDetectorResult.h>

class ShowDraw::ShowDrawCanvasDetectorBridge::Impl {
public:
    ShowDrawCanvasDetectorWrapper* detector;

    Impl() {
        detector = [[ShowDrawCanvasDetectorWrapper alloc] init];
    }

    std::vector<ShowDraw::CanvasDetectorResult> detect(const uint8_t* bgra_data, int width, int height) {
        if (!detector) {
            return {};
        }

        CVPixelBufferRef pixelBuffer = nullptr;
        NSDictionary* pixelBufferOptions = @{
            (id)kCVPixelBufferCGImageCompatibilityKey: @YES,
            (id)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
            (id)kCVPixelBufferWidthKey: @(width),
            (id)kCVPixelBufferHeightKey: @(height),
            (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)
        };

        CVReturn status = CVPixelBufferCreateWithBytes(kCFAllocatorDefault, width, height, kCVPixelFormatType_32BGRA, (void*)bgra_data, width * 4, nullptr, nullptr, (__bridge CFDictionaryRef)pixelBufferOptions, &pixelBuffer);

        if (status != kCVReturnSuccess) {
            // TODO: Log error
            return {};
        }

        VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithCVPixelBuffer:pixelBuffer options:@{}];
        if (!handler) {
            CVPixelBufferRelease(pixelBuffer);
            return {};
        }

        NSError* error = nil;
        NSArray<ShowDrawCanvasDetectorResult*>* results = [detector detectionsForHandler:handler withSize:CGSizeMake(width, height) error:&error];
        CVPixelBufferRelease(pixelBuffer);

        if (error) {
            // TODO: Log error
            NSLog(@"Detection error: %@", error);
            return {};
        }

        std::vector<ShowDraw::CanvasDetectorResult> cpp_results;
        for (ShowDrawCanvasDetectorResult* result in results) {
            ShowDraw::CanvasDetectorResult cpp_result;
            cpp_result.confidence = result.confidence;
            CGRect boundingBox = result.boundingBox;
            cpp_result.x = boundingBox.origin.x;
            cpp_result.y = boundingBox.origin.y;
            cpp_result.width = boundingBox.size.width;
            cpp_result.height = boundingBox.size.height;
            cpp_results.push_back(cpp_result);
        }

        return cpp_results;
    }
};

ShowDraw::ShowDrawCanvasDetectorBridge::ShowDrawCanvasDetectorBridge() : pimpl(std::make_unique<Impl>()) {}
ShowDraw::ShowDrawCanvasDetectorBridge::~ShowDrawCanvasDetectorBridge() = default;
ShowDraw::ShowDrawCanvasDetectorBridge::ShowDrawCanvasDetectorBridge(ShowDrawCanvasDetectorBridge&&) noexcept = default;
ShowDraw::ShowDrawCanvasDetectorBridge& ShowDraw::ShowDrawCanvasDetectorBridge::operator=(ShowDrawCanvasDetectorBridge&&) noexcept = default;

std::vector<ShowDraw::CanvasDetectorResult> ShowDraw::ShowDrawCanvasDetectorBridge::detect(const uint8_t* bgra_data, int width, int height) {
    return pimpl->detect(bgra_data, width, height);
}
