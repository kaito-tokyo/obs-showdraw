#import "ShowDrawCanvasDetectorWrapper.h"
#import "ShowDrawCanvasDetectorModel.h"
#import "ShowDrawCanvasDetectorResult.h"

//------------------------------------------------------------------------------
#pragma mark - Constants
//------------------------------------------------------------------------------

/// The input width of the Core ML model in pixels.
static const CGFloat ModelInputWidth = 640.0;
/// The input height of the Core ML model in pixels.
static const CGFloat ModelInputHeight = 640.0;

/**
 * The error domain for errors created by `ShowDrawCanvasDetectorWrapper`.
 */
NSString *const ShowDrawCanvasDetectorErrorDomain = @"tokyo.kaito.ShowDrawCanvasDetectorWrapper.ErrorDomain";

/**
 * Error codes for errors created by `ShowDrawCanvasDetectorWrapper`.
 */
typedef NS_ENUM(NSInteger, ShowDrawCanvasDetectorErrorCode) {
    /// An unknown error occurred.
    ShowDrawCanvasDetectorErrorUnknown = -1,
    /// The Vision request succeeded but the model returned no results.
    ShowDrawCanvasDetectorErrorNoResults = 1,
    /// The model's output did not contain the expected feature tensors.
    ShowDrawCanvasDetectorErrorMissingTensors = 2,
};

//------------------------------------------------------------------------------
#pragma mark - Class Extension
//------------------------------------------------------------------------------

@interface ShowDrawCanvasDetectorWrapper ()

/// The underlying Vision Core ML model used for performing predictions.
@property (nonatomic, strong) VNCoreMLModel *visionModel;

/**
 * Converts bounding box coordinates from the model's output space to the original image's pixel space.
 * @discussion This method reverses the letterboxing (scaling and padding) applied by Vision's `VNImageCropAndScaleOptionScaleFit` option.
 *
 * @param modelCenter The center point (x, y) of the bounding box from the model's output.
 * @param modelSize The size (width, height) of the bounding box from the model's output.
 * @param scale The scale factor that was used to fit the image into the model's input dimensions.
 * @param padding The padding that was added to the image on the x and y axes.
 * @return A `CGRect` representing the bounding box in the original image's pixel coordinate system.
 */
- (CGRect)pixelBoundingBoxForModelCoordinates:(CGPoint)modelCenter
                                         size:(CGSize)modelSize
                                        scale:(CGFloat)scale
                                      padding:(CGPoint)padding;
@end

//------------------------------------------------------------------------------
#pragma mark - Implementation
//------------------------------------------------------------------------------

@implementation ShowDrawCanvasDetectorWrapper

- (instancetype)init
{
    self = [super init];
    if (self) {
        ShowDrawCanvasDetectorModel *model = [[ShowDrawCanvasDetectorModel alloc] init];
        if (!model) {
            return nil;
        }

        NSError *error = nil;
        _visionModel = [VNCoreMLModel modelForMLModel:model.model error:&error];
        if (error || !_visionModel) {
            NSLog(@"Failed to create VNCoreMLModel: %@", error);
            return nil;
        }
    }
    return self;
}

- (nullable NSArray<ShowDrawCanvasDetectorResult *> *)detectionsForHandler:(VNImageRequestHandler *)handler
                                                                  withSize:(CGSize)imageSize
                                                                     error:(NSError **)error;
{
    VNCoreMLRequest *request = [[VNCoreMLRequest alloc] initWithModel:self.visionModel];
    request.imageCropAndScaleOption = VNImageCropAndScaleOptionScaleFit;

    if (![handler performRequests:@[request] error:error]) {
        return nil;
    }

    if (!request.results || request.results.count == 0) {
        if (error) {
            NSDictionary *userInfo =
                @{NSLocalizedDescriptionKey: @"Vision request succeeded, but returned no results from the model."};
            *error = [NSError errorWithDomain:ShowDrawCanvasDetectorErrorDomain
                                         code:ShowDrawCanvasDetectorErrorNoResults
                                     userInfo:userInfo];
        }
        return nil;
    }

    MLMultiArray *confidenceTensor = nil;
    MLMultiArray *coordinatesTensor = nil;
    for (VNCoreMLFeatureValueObservation *observation in request.results) {
        if ([observation.featureName isEqualToString:@"confidence"]) {
            confidenceTensor = observation.featureValue.multiArrayValue;
        } else if ([observation.featureName isEqualToString:@"coordinates"]) {
            coordinatesTensor = observation.featureValue.multiArrayValue;
        }
    }

    if (!confidenceTensor || !coordinatesTensor) {
        if (error) {
            NSDictionary *userInfo = @{
                NSLocalizedDescriptionKey:
                    @"Could not find expected output tensors ('confidence' and/or 'coordinates') in the model's results."
            };
            *error = [NSError errorWithDomain:ShowDrawCanvasDetectorErrorDomain
                                         code:ShowDrawCanvasDetectorErrorMissingTensors
                                     userInfo:userInfo];
        }
        return nil;
    }

    NSMutableArray<ShowDrawCanvasDetectorResult *> *detections = [NSMutableArray array];

    CGFloat imageWidth = imageSize.width;
    CGFloat imageHeight = imageSize.height;
    CGFloat scale = fmin(ModelInputWidth / imageWidth, ModelInputHeight / imageHeight);
    CGFloat scaledWidth = imageWidth * scale;
    CGFloat scaledHeight = imageHeight * scale;
    CGPoint padding = CGPointMake((ModelInputWidth - scaledWidth) / 2.0, (ModelInputHeight - scaledHeight) / 2.0);

    int maxBoxes = [coordinatesTensor.shape[[coordinatesTensor.shape count] - 2] intValue];

    for (int i = 0; i < maxBoxes; i++) {
        NSNumber *confidence = confidenceTensor[@[@(i), @(0)]];
        if (confidence.floatValue == 0.0) {
            continue;
        }

        CGPoint modelCenter =
            CGPointMake([coordinatesTensor[@[@(i), @(0)]] floatValue], [coordinatesTensor[@[@(i), @(1)]] floatValue]);
        CGSize modelSize =
            CGSizeMake([coordinatesTensor[@[@(i), @(2)]] floatValue], [coordinatesTensor[@[@(i), @(3)]] floatValue]);

        CGRect pixelBoundingBox = [self pixelBoundingBoxForModelCoordinates:modelCenter size:modelSize scale:scale
                                                                    padding:padding];

        ShowDrawCanvasDetectorResult *result = [[ShowDrawCanvasDetectorResult alloc]
            initWithBoundingBox:pixelBoundingBox
                     confidence:confidence.floatValue];
        [detections addObject:result];
    }

    return [detections copy];
}

#pragma mark - Private Methods

- (CGRect)pixelBoundingBoxForModelCoordinates:(CGPoint)modelCenter
                                         size:(CGSize)modelSize
                                        scale:(CGFloat)scale
                                      padding:(CGPoint)padding
{
    CGFloat centerX_no_pad = modelCenter.x - padding.x;
    CGFloat centerY_no_pad = modelCenter.y - padding.y;

    CGFloat original_centerX = centerX_no_pad / scale;
    CGFloat original_centerY = centerY_no_pad / scale;
    CGFloat original_width = modelSize.width / scale;
    CGFloat original_height = modelSize.height / scale;

    CGFloat original_x = original_centerX - (original_width / 2.0);
    CGFloat original_y = original_centerY - (original_height / 2.0);

    return CGRectMake(original_x, original_y, original_width, original_height);
}

@end
