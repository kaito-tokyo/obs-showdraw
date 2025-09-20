#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>

NS_ASSUME_NONNULL_BEGIN

@interface ShowDrawCanvasDetectorWrapper : NSObject

// 戻り値の型を実際に返ってくる型に修正
- (nullable NSArray<VNCoreMLFeatureValueObservation *> *)performDetectionWithHandler:(VNImageRequestHandler *)handler error:(NSError **)error;

@end

NS_ASSUME_NONNULL_END
