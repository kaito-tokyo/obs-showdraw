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

#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>
#import "ShowDrawCanvasDetectorResult.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * A wrapper class for the `ShowDrawCanvasDetector` Core ML model.
 *
 * This class simplifies performing object detection by handling the model loading,
 * request creation, and post-processing of the raw tensor outputs.
 */
@interface ShowDrawCanvasDetectorWrapper : NSObject

/**
 * Returns an array of detected objects from the image provided via the image request handler.
 *
 * This method executes the Core ML model, parses the resulting 'confidence' and 'coordinates'
 * tensors, and converts them into `ShowDrawCanvasDetectorResult` objects. In Swift, this
 * method is imported as `detections(forHandler:withSize:)` and can throw an error.
 *
 * @param handler A `VNImageRequestHandler` initialized with the image data on which to perform detection.
 * @param imageSize The original size of the image being processed. This is required to correctly
 * scale the bounding box coordinates from the model's output space to the image's pixel space.
 * @param error An out-parameter that will be populated with an `NSError` object if an error occurs.
 * @return An array of `ShowDrawCanvasDetectorResult` objects on success, or `nil` on failure.
 */
- (nullable NSArray<ShowDrawCanvasDetectorResult *> *)detectionsForHandler:(VNImageRequestHandler *)handler
                                                                 withSize:(CGSize)imageSize
                                                                    error:(NSError **)error;
@end

NS_ASSUME_NONNULL_END
