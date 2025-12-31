//
//  JITManager.h
//  Play! - JIT Manager for iOS 26
//

#import <Foundation/Foundation.h>
#import "DualMapping.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, JITStatus) {
    JITStatusDisabled = 0,
    JITStatusEnabled = 1,
    JITStatusPending = 2,
    JITStatusError = -1
};

@interface JITManager : NSObject

@property (nonatomic, readonly) JITStatus status;
@property (nonatomic, readonly) BOOL isJITEnabled;

+ (instancetype)sharedManager;

- (BOOL)initializeJIT;
- (void)checkJITStatus;
- (DualMappedRegion*)allocateJITRegionWithSize:(size_t)size;
- (void)freeJITRegion:(DualMappedRegion*)region;
- (void*)compileAndWrite:(const void*)code 
                  length:(size_t)length 
                toRegion:(DualMappedRegion*)region;
- (void)waitForJITActivation;
- (void)notifyJITActivated;

@end

NS_ASSUME_NONNULL_END

