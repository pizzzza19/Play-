//
//  DualMapping.h
//  Play! - Dual Mapping JIT for iOS 26
//

#import <Foundation/Foundation.h>
#import <mach/mach.h>
#import <sys/mman.h>

NS_ASSUME_NONNULL_BEGIN

typedef struct {
    void* rwAddress;
    void* rxAddress;
    size_t size;
    int fd;
} DualMappedRegion;

@interface DualMapping : NSObject

+ (instancetype)sharedInstance;

- (DualMappedRegion*)createDualMappedRegionWithSize:(size_t)size;
- (void)freeDualMappedRegion:(DualMappedRegion*)region;
- (BOOL)writeCode:(const void*)code 
           length:(size_t)length 
         toRegion:(DualMappedRegion*)region 
           offset:(size_t)offset;
- (void*)getExecutablePointer:(DualMappedRegion*)region offset:(size_t)offset;
- (BOOL)isJITAvailable;
- (BOOL)enableDebugMode;

@end

NS_ASSUME_NONNULL_END
