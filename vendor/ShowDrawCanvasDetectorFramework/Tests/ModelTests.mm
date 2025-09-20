#import <XCTest/XCTest.h>
#import "ShowDrawCanvasDetectorWrapper.h"
#import <AppKit/AppKit.h>

#include <vector>

// 検出結果を格納するためのシンプルなカスタムクラス
@interface Detection : NSObject
@property (nonatomic) CGRect boundingBox;
@property (nonatomic) float confidence;
@end
@implementation Detection
@end


@interface ModelTests : XCTestCase
@property (nonatomic, strong) ShowDrawCanvasDetectorWrapper *wrapper;
@end

@implementation ModelTests

- (void)setUp {
    [super setUp];
    self.wrapper = [[ShowDrawCanvasDetectorWrapper alloc] init];
    XCTAssertNotNil(self.wrapper, @"ラッパーの初期化に失敗しました");
}

- (void)testDetectionAndPostprocess {
    // 1. 画像の準備
    NSBundle *testBundle = [NSBundle bundleForClass:[self class]];
    NSString *imagePath = [testBundle pathForResource:@"valid001" ofType:@"jpg"];
    XCTAssertNotNil(imagePath, @"テスト画像が見つかりません");
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:imagePath];
    XCTAssertNotNil(image, @"テスト画像の読み込みに失敗しました");
    
    CGRect imageRect = CGRectMake(0, 0, image.size.width, image.size.height);
    CGImageRef cgImage = [image CGImageForProposedRect:&imageRect context:nil hints:nil];
    XCTAssertTrue(cgImage != NULL, @"CGImageへの変換に失敗しました");

    // 2. Visionリクエストの実行
    VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCGImage:cgImage options:@{}];
    NSError *error = nil;
    NSArray<VNCoreMLFeatureValueObservation *> *results = [self.wrapper performDetectionWithHandler:handler error:&error];

    // 3. 生の出力テンソルを取得
    XCTAssertNil(error, @"推論エラー: %@", error.localizedDescription);
    MLMultiArray *outputTensor = results.firstObject.featureValue.multiArrayValue;
    XCTAssertNotNil(outputTensor, @"出力テンソルが取得できませんでした");
    
    // 4. 後処理を実行し、結果を配列に格納
    NSLog(@"--- Post-processing and Sorting Detections ---");
    
    float *dataPointer = (float *)outputTensor.dataPointer;
    NSUInteger dataStride = outputTensor.strides.lastObject.unsignedIntegerValue;
    
    const float confidenceThreshold = 0.3; // 信頼度の閾値
    
    const std::vector<int> strides = {8, 16, 32};
    const std::vector<int> num_grids_per_stride = {80 * 80, 40 * 40, 20 * 20};

    auto sigmoid = [](float x) { return 1.0f / (1.0f + expf(-x)); };

    NSMutableArray<Detection *> *detections = [NSMutableArray array];
    
    int grid_idx_offset = 0;
    for (int s = 0; s < strides.size(); ++s) {
        int current_stride = strides[s];
        int num_grid = num_grids_per_stride[s];
        int grid_size_w = sqrt(num_grid);

        for (int i = 0; i < num_grid; i++) {
            int grid_y = i / grid_size_w;
            int grid_x = i % grid_size_w;
            int data_idx = (grid_idx_offset + i) * dataStride;
            
            float obj_conf = dataPointer[data_idx + 4];
            float obj_prob = sigmoid(obj_conf);

            if (obj_prob <= confidenceThreshold) continue;
            
            float cls_conf = dataPointer[data_idx + 5];
            float cls_prob = sigmoid(cls_conf);
            float confidence = obj_prob * cls_prob;

            if (confidence <= confidenceThreshold) continue;
            
            float x_center_logit = dataPointer[data_idx + 0];
            float y_center_logit = dataPointer[data_idx + 1];
            float width_logit    = dataPointer[data_idx + 2];
            float height_logit   = dataPointer[data_idx + 3];

            float x_center = (x_center_logit + grid_x) * current_stride;
            float y_center = (y_center_logit + grid_y) * current_stride;
            float width    = expf(width_logit) * current_stride;
            float height   = expf(height_logit) * current_stride;
            
            float x0 = x_center - width / 2.0f;
            float y0 = y_center - height / 2.0f;

            Detection *det = [[Detection alloc] init];
            det.boundingBox = CGRectMake(x0, y0, width, height);
            det.confidence = confidence;
            [detections addObject:det];
        }
        grid_idx_offset += num_grid;
    }

    // 5. ★★ 信頼度で配列をソート (降順) ★★
    [detections sortUsingComparator:^NSComparisonResult(Detection *obj1, Detection *obj2) {
        if (obj1.confidence > obj2.confidence) {
            return NSOrderedAscending;
        } else if (obj1.confidence < obj2.confidence) {
            return NSOrderedDescending;
        }
        return NSOrderedSame;
    }];

    // 6. ソート後の結果を出力
    NSLog(@"--- Found %lu detections (sorted by confidence) ---", (unsigned long)detections.count);
    for (Detection *det in detections) {
        CGRect box = det.boundingBox;
        NSLog(@"Box: [x0: %.2f, y0: %.2f, w: %.2f, h: %.2f, conf: %.4f]",
              box.origin.x, box.origin.y, box.size.width, box.size.height, det.confidence);
    }
}

@end
