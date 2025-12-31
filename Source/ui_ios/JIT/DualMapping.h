//
//  DualMapping.h
//  Play! - Dual Mapping JIT Implementation
//

#import <Foundation/Foundation.h>
#import <mach/mach.h>
#import <sys/mman.h>

NS_ASSUME_NONNULL_BEGIN

@interface DualMapping : NSObject

// Structure pour stocker les mappings
typedef struct {
    void* rwAddress;  // Read-Write address
    void* rxAddress;  // Read-Execute address
    size_t size;
    int fd;
} DualMappedRegion;

// Initialisation
+ (instancetype)sharedInstance;

// Créer une région dual-mapped
- (DualMappedRegion*)createDualMappedRegionWithSize:(size_t)size;

// Libérer une région
- (void)freeDualMappedRegion:(DualMappedRegion*)region;

// Écrire du code dans la région RW
- (BOOL)writeCode:(const void*)code 
           length:(size_t)length 
         toRegion:(DualMappedRegion*)region 
           offset:(size_t)offset;

// Obtenir le pointeur RX pour l'exécution
- (void*)getExecutablePointer:(DualMappedRegion*)region offset:(size_t)offset;

// Vérifier si JIT est disponible
- (BOOL)isJITAvailable;

// Activer le mode debug (nécessaire pour iOS 26)
- (BOOL)enableDebugMode;

@end

NS_ASSUME_NONNULL_END
