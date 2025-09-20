#import "ShowDrawCanvasDetectorWrapper.h"
#import "ShowDrawCanvasDetectorModel.h"

@interface ShowDrawCanvasDetectorWrapper ()
@property (nonatomic, strong) VNCoreMLModel *visionModel;
@end

@implementation ShowDrawCanvasDetectorWrapper

- (instancetype)init {
    self = [super init];
    if (self) {
        ShowDrawCanvasDetectorModel *model = [[ShowDrawCanvasDetectorModel alloc] init];
        if (!model) { return nil; }

        NSError *error = nil;
        _visionModel = [VNCoreMLModel modelForMLModel:model.model error:&error];
        if (error) {
            NSLog(@"Failed to create VNCoreMLModel: %@", error);
            return nil;
        }
    }
    return self;
}

// 戻り値の型を VNCoreMLFeatureValueObservation に修正
- (nullable NSArray<VNCoreMLFeatureValueObservation *> *)performDetectionWithHandler:(VNImageRequestHandler *)handler error:(NSError **)error {
    VNCoreMLRequest *request = [[VNCoreMLRequest alloc] initWithModel:self.visionModel];
    request.imageCropAndScaleOption = VNImageCropAndScaleOptionScaleFill;

    if (![handler performRequests:@[request] error:error]) {
        return nil;
    }
    
    // 生のテンソルを返すので、このキャストが正しい
    return (NSArray<VNCoreMLFeatureValueObservation *> *)request.results;
}
@end
