/*
ShowDraw
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "ShowDrawCanvasDetectorBridge.hpp"

#import <CoreML/CoreML.h>
#import <Vision/Vision.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreGraphics/CoreGraphics.h>

#import <CoreMLShowDrawCanvasDetector/ShowDrawCanvasDetectorWrapper.h>
#import <CoreMLShowDrawCanvasDetector/ShowDrawCanvasDetectorResult.h>

namespace KaitoTokyo {
    namespace ShowDraw {

        class ShowDrawCanvasDetectorBridge::Impl
        {
              public:
            ShowDrawCanvasDetectorWrapper *detector;

            Impl()
            {
                detector = [[ShowDrawCanvasDetectorWrapper alloc] init];
            }

            std::vector<ShowDraw::CanvasDetectorResult> detect(const uint8_t *bgra_data, int width, int height)
            {
                if (!detector) {
                    return {};
                }

                CVPixelBufferRef pixelBuffer = nullptr;
                NSDictionary *pixelBufferOptions = @{
                    (id) kCVPixelBufferCGImageCompatibilityKey: @YES,
                    (id) kCVPixelBufferCGBitmapContextCompatibilityKey: @YES,
                    (id) kCVPixelBufferWidthKey: @(width),
                    (id) kCVPixelBufferHeightKey: @(height),
                    (id) kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)
                };

                CVReturn status = CVPixelBufferCreateWithBytes(
                    kCFAllocatorDefault, width, height, kCVPixelFormatType_32BGRA, (void *) bgra_data, width * 4,
                    nullptr, nullptr, (__bridge CFDictionaryRef) pixelBufferOptions, &pixelBuffer);

                if (status != kCVReturnSuccess) {
                    // TODO: Log error
                    return {};
                }

                VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCVPixelBuffer:pixelBuffer
                                                                                              options:@ {}];
                if (!handler) {
                    CVPixelBufferRelease(pixelBuffer);
                    return {};
                }

                NSError *error = nil;
                NSArray<ShowDrawCanvasDetectorResult *> *results =
                    [detector detectionsForHandler:handler withSize:CGSizeMake(width, height) error:&error];
                CVPixelBufferRelease(pixelBuffer);

                if (error) {
                    // TODO: Log error
                    NSLog(@"Detection error: %@", error);
                    return {};
                }

                std::vector<CanvasDetectorResult> cpp_results;
                for (ShowDrawCanvasDetectorResult *result in results) {
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

        ShowDrawCanvasDetectorBridge::ShowDrawCanvasDetectorBridge() : pimpl(std::make_unique<Impl>()) {}
        ShowDrawCanvasDetectorBridge::~ShowDrawCanvasDetectorBridge() = default;
        ShowDrawCanvasDetectorBridge::ShowDrawCanvasDetectorBridge(ShowDrawCanvasDetectorBridge &&) noexcept = default;
        ShowDrawCanvasDetectorBridge &
            ShowDrawCanvasDetectorBridge::operator=(ShowDrawCanvasDetectorBridge &&) noexcept = default;

        std::vector<CanvasDetectorResult> ShowDrawCanvasDetectorBridge::detect(const uint8_t *bgra_data, int width,
                                                                               int height)
        {
            return pimpl->detect(bgra_data, width, height);
        }

    }  // namespace ShowDraw
}  // namespace KaitoTokyo
