//
//  JITManager.h
//  Play! - JIT Manager
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

// Singleton
+ (instancetype)sharedManager;

// Initialiser le JIT
- (BOOL)initializeJIT;

// Vérifier le statut
- (void)checkJITStatus;

// Allouer une région JIT
- (DualMappedRegion*)allocateJITRegionWithSize:(size_t)size;

// Libérer une région
- (void)freeJITRegion:(DualMappedRegion*)region;

// Compiler et écrire du code
- (void*)compileAndWrite:(const void*)code 
                  length:(size_t)length 
                toRegion:(DualMappedRegion*)region;

// Pour l'intégration avec StikDebug
- (void)waitForJITActivation;
- (void)notifyJITActivated;

@end

NS_ASSUME_NONNULL_END
