//
//  JITManager.mm
//  Play! - JIT Manager
//

#import "JITManager.h"
#import <UIKit/UIKit.h>

@interface JITManager ()
@property (nonatomic, strong) DualMapping *dualMapping;
@property (nonatomic, strong) NSMutableArray<NSValue*> *allocatedRegions;
@property (nonatomic, assign) JITStatus status;
@property (nonatomic, strong) dispatch_semaphore_t jitSemaphore;
@end

@implementation JITManager

+ (instancetype)sharedManager {
    static JITManager *manager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        manager = [[self alloc] init];
    });
    return manager;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _dualMapping = [DualMapping sharedInstance];
        _allocatedRegions = [NSMutableArray array];
        _status = JITStatusDisabled;
        _jitSemaphore = dispatch_semaphore_create(0);
        
        // Observer pour l'activation JIT
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleJITActivation:)
                                                     name:@"JITActivatedNotification"
                                                   object:nil];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)initializeJIT {
    NSLog(@"[JITManager] Initializing JIT system...");
    
    // Vérifier si JIT est disponible
    if (![self.dualMapping isJITAvailable]) {
        NSLog(@"[JITManager] JIT is not available on this device/iOS version");
        
        // Afficher une alerte si on est sur le thread principal
        if ([NSThread isMainThread]) {
            [self showJITWarning];
        } else {
            dispatch_async(dispatch_get_main_queue(), ^{
                [self showJITWarning];
            });
        }
        
        self.status = JITStatusPending;
        return NO;
    }
    
    self.status = JITStatusEnabled;
    NSLog(@"[JITManager] JIT initialized successfully");
    return YES;
}

- (void)checkJITStatus {
    if ([self.dualMapping isJITAvailable]) {
        self.status = JITStatusEnabled;
    } else {
        self.status = JITStatusDisabled;
    }
}

- (BOOL)isJITEnabled {
    return self.status == JITStatusEnabled;
}

- (DualMappedRegion*)allocateJITRegionWithSize:(size_t)size {
    if (!self.isJITEnabled) {
        NSLog(@"[JITManager] Cannot allocate: JIT not enabled");
        return NULL;
    }
    
    DualMappedRegion *region = [self.dualMapping createDualMappedRegionWithSize:size];
    
    if (region) {
        [self.allocatedRegions addObject:[NSValue valueWithPointer:region]];
        NSLog(@"[JITManager] Allocated JIT region of %zu bytes", size);
    }
    
    return region;
}

- (void)freeJITRegion:(DualMappedRegion*)region {
    if (!region) return;
    
    [self.dualMapping freeDualMappedRegion:region];
    
    NSValue *value = [NSValue valueWithPointer:region];
    [self.allocatedRegions removeObject:value];
    
    NSLog(@"[JITManager] Freed JIT region");
}

- (void*)compileAndWrite:(const void*)code 
                  length:(size_t)length 
                toRegion:(DualMappedRegion*)region {
    
    if (!region || !code) {
        return NULL;
    }
    
    // Écrire le code dans la région RW
    BOOL success = [self.dualMapping writeCode:code 
                                        length:length 
                                      toRegion:region 
                                        offset:0];
    
    if (!success) {
        NSLog(@"[JITManager] Failed to write code to JIT region");
        return NULL;
    }
    
    // Retourner le pointeur exécutable (RX)
    return [self.dualMapping getExecutablePointer:region offset:0];
}

- (void)waitForJITActivation {
    if (self.isJITEnabled) {
        return; // Déjà activé
    }
    
    NSLog(@"[JITManager] Waiting for JIT activation...");
    
    // Attendre max 60 secondes
    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 60 * NSEC_PER_SEC);
    long result = dispatch_semaphore_wait(self.jitSemaphore, timeout);
    
    if (result != 0) {
        NSLog(@"[JITManager] JIT activation timeout!");
    }
}

- (void)notifyJITActivated {
    NSLog(@"[JITManager] JIT has been activated!");
    self.status = JITStatusEnabled;
    dispatch_semaphore_signal(self.jitSemaphore);
}

- (void)handleJITActivation:(NSNotification*)notification {
    [self notifyJITActivated];
}

- (void)showJITWarning {
    UIAlertController *alert = [UIAlertController 
        alertControllerWithTitle:@"JIT Required"
        message:@"Play! requires JIT compilation for optimal performance.\n\n"
                @"Please use StikDebug or AltStore to enable JIT.\n\n"
                @"The app will wait for JIT activation..."
        preferredStyle:UIAlertControllerStyleAlert];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" 
                                              style:UIAlertActionStyleDefault 
                                            handler:nil]];
    
    UIViewController *rootVC = [UIApplication sharedApplication].keyWindow.rootViewController;
    if (rootVC) {
        [rootVC presentViewController:alert animated:YES completion:nil];
    }
}

@end
