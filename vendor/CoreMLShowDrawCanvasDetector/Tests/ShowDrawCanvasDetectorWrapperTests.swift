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

import XCTest
import AppKit

@testable import CoreMLShowDrawCanvasDetector

final class ModelTests: XCTestCase {

    var wrapper: ShowDrawCanvasDetectorWrapper!

    override func setUpWithError() throws {
        // `throws`キーワードにより、セットアップ中に発生したエラーをテストフレームワークが検知します
        try super.setUpWithError()
        
        // Objective-Cのクラスを通常通り初期化します
        let wrapperInstance = ShowDrawCanvasDetectorWrapper()
        XCTAssertNotNil(wrapperInstance, "Wrapper initialization failed.")
        self.wrapper = wrapperInstance
    }
    
    func testDetectionAndDraw() throws {
        // 1. ヘルパーメソッドを使ってテスト画像をロード
        let image = try loadImage(named: "test001", ofType: "jpg")
        
        // 2. Visionで扱うために画像を準備
        var imageRect = CGRect(origin: .zero, size: image.size)
        guard let cgImage = image.cgImage(forProposedRect: &imageRect, context: nil, hints: nil) else {
            XCTFail("Failed to convert NSImage to CGImage.")
            return
        }

        // 3. 検出を実行
        let handler = VNImageRequestHandler(cgImage: cgImage, options: [:])
        
        // Objective-Cの `error:` パラメータ付きメソッドは、Swiftでは `throws` メソッドとして扱われます。
        // そのため `try` を使って呼び出します。
        let results = try wrapper.detections(for: handler, with: image.size)
        
        // 4. 結果をアサート（検証）
        XCTAssertFalse(results.isEmpty, "No objects were detected.")

        // 5. 検出結果のバウンディングボックスを描画して画像を保存
        let outputImage = try drawBoxes(on: image, from: results)
        try save(image: outputImage, to: "detected_image.png")
    }

    // MARK: - Helper Methods
    
    private func loadImage(named name: String, ofType fileExtension: String) throws -> NSImage {
        // This now correctly calls Swift's global type(of:) function
        let bundle = Bundle(for: type(of: self))
        
        // Use the new parameter name here as well
        guard let path = bundle.path(forResource: name, ofType: fileExtension) else {
            throw TestError.fileNotFound("\(name).\(fileExtension)")
        }
        
        guard let image = NSImage(contentsOfFile: path) else {
            throw TestError.imageLoadingFailed(path)
        }
        
        return image
    }
    
    private func drawBoxes(on image: NSImage, from results: [ShowDrawCanvasDetectorResult]) throws -> NSImage {
        guard let newImage = image.copy() as? NSImage else {
            throw TestError.imageProcessingFailed("Failed to copy image.")
        }
        
        newImage.lockFocus()
        for result in results {
            let path = NSBezierPath(rect: result.boundingBox)
            NSColor.red.withAlphaComponent(0.8).setStroke()
            path.lineWidth = 2.0
            path.stroke()
        }
        newImage.unlockFocus()
        
        return newImage
    }
    
    private func save(image: NSImage, to fileName: String) throws {
        let desktopURL = try FileManager.default.url(for: .desktopDirectory, in: .userDomainMask, appropriateFor: nil, create: true)
        let outputURL = desktopURL.appendingPathComponent(fileName)
        
        guard let tiffData = image.tiffRepresentation,
              let bitmap = NSBitmapImageRep(data: tiffData),
              let pngData = bitmap.representation(using: .png, properties: [:]) else {
            throw TestError.imageProcessingFailed("Failed to convert image to PNG data.")
        }
        
        try pngData.write(to: outputURL)
        print("Successfully saved detected image to: \(outputURL.path)")
    }
    
    // テスト用のカスタムエラー
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
