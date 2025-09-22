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
#import <CoreGraphics/CoreGraphics.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * A data class that holds the result of a single object detection.
 *
 * This class encapsulates the bounding box in pixel coordinates and the confidence score.
 */
@interface ShowDrawCanvasDetectorResult : NSObject

/// The bounding box of the detected object in pixel coordinates.
@property (nonatomic, assign) CGRect boundingBox;

/// The confidence score of the detection, ranging from 0.0 to 1.0.
@property (nonatomic, assign) float confidence;

/**
 * Initializes a new result object with the specified bounding box and confidence score.
 *
 * @param box The bounding box of the detected object.
 * @param confidence The confidence score of the detection.
 * @return An initialized `ShowDrawCanvasDetectorResult` object.
 */
- (instancetype)initWithBoundingBox:(CGRect)box confidence:(float)confidence;

@end

NS_ASSUME_NONNULL_END
