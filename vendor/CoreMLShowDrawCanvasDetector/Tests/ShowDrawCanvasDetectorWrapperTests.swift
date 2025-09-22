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

import AppKit
import XCTest

@testable import CoreMLShowDrawCanvasDetector

final class ShowDrawCanvasDetectorTests: XCTestCase {

  var wrapper: ShowDrawCanvasDetectorWrapper!

  override func setUpWithError() throws {
    try super.setUpWithError()
    wrapper = ShowDrawCanvasDetectorWrapper()
    XCTAssertNotNil(wrapper, "Wrapper initialization failed.")
  }

  /// Tests the detection results against known, constant values to ensure accuracy.
  func testDetectionResults_againstKnownValues() throws {
    // MARK: 1. Define Ground Truth Data
    // TODO: Replace these values with the actual results from an initial test run.
    let expectedDetectionsCount = 1
    let expectedTopResult = (
      rect: CGRect(x: 704.0625, y: 52.5, width: 744.375, height: 973.5),
      confidence: 0.9399414
    )
    // Define the acceptable margin of error for floating-point comparisons.
    let accuracy: CGFloat = 0.1

    // MARK: 2. Prepare Test Image and Perform Inference
    let image = try loadImage(named: "test001", ofType: "jpg")

    var imageRect = CGRect(origin: .zero, size: image.size)
    guard let cgImage = image.cgImage(forProposedRect: &imageRect, context: nil, hints: nil) else {
      XCTFail("Failed to convert NSImage to CGImage.")
      return
    }
    let handler = VNImageRequestHandler(cgImage: cgImage, options: [:])
    let results = try wrapper.detections(for: handler, with: image.size)

    // MARK: 3. Assert the Results

    // Check if the number of detected objects matches the expected count.
    XCTAssertEqual(
      results.count, expectedDetectionsCount,
      "Expected \(expectedDetectionsCount) detections, but found \(results.count).")

    // Get the result with the highest confidence to ensure the test is stable even if the model's output order changes.
    guard let topResult = results.max(by: { $0.confidence < $1.confidence }) else {
      XCTFail("Could not get the top result from detections.")
      return
    }

    // Compare each value against the expected ground truth within the defined accuracy.
    XCTAssertEqual(
      topResult.boundingBox.origin.x, expectedTopResult.rect.origin.x, accuracy: accuracy)
    XCTAssertEqual(
      topResult.boundingBox.origin.y, expectedTopResult.rect.origin.y, accuracy: accuracy)
    XCTAssertEqual(
      topResult.boundingBox.size.width, expectedTopResult.rect.size.width, accuracy: accuracy)
    XCTAssertEqual(
      topResult.boundingBox.size.height, expectedTopResult.rect.size.height, accuracy: accuracy)
    XCTAssertEqual(
      Float(topResult.confidence), Float(expectedTopResult.confidence), accuracy: Float(accuracy))
  }

  // MARK: - Helper Methods

  private func loadImage(named name: String, ofType fileExtension: String) throws -> NSImage {
    let bundle = Bundle(for: type(of: self))

    guard let path = bundle.path(forResource: name, ofType: fileExtension) else {
      throw TestError.fileNotFound("\(name).\(fileExtension)")
    }

    guard let image = NSImage(contentsOfFile: path) else {
      throw TestError.imageLoadingFailed(path)
    }

    return image
  }

  /// Custom errors for testing purposes.
  enum TestError: Error, LocalizedError {
    case fileNotFound(String)
    case imageLoadingFailed(String)
    case imageProcessingFailed(String)

    var errorDescription: String? {
      switch self {
      case .fileNotFound(let name): return "Test resource not found: \(name)"
      case .imageLoadingFailed(let path): return "Failed to load image at path: \(path)"
      case .imageProcessingFailed(let message): return "Image processing failed: \(message)"
      }
    }
  }
}
