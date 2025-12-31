//
//  DualMapping.mm
//  Play! - Dual Mapping JIT Implementation
//

#import "DualMapping.h"
#import <sys/ptrace.h>
#import <dlfcn.h>
#import <pthread.h>

@implementation DualMapping

+ (instancetype)sharedInstance {
    static DualMapping *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        // Activer le mode debug au démarrage
        [self enableDebugMode];
    }
    return self;
}

- (BOOL)enableDebugMode {
    // Méthode 1: ptrace pour iOS < 17
    int ret = ptrace(PT_TRACE_ME, 0, NULL, 0);
    
    if (ret == 0) {
        NSLog(@"[DualMapping] Debug mode enabled via ptrace");
        return YES;
    }
    
    // Méthode 2: Pour iOS 17+, on utilise task_for_pid si disponible
    // Ceci nécessite get-task-allow entitlement
    mach_port_t task;
    kern_return_t kr = task_for_pid(mach_task_self(), getpid(), &task);
    
    if (kr == KERN_SUCCESS) {
        NSLog(@"[DualMapping] Debug mode enabled via task_for_pid");
        return YES;
    }
    
    NSLog(@"[DualMapping] Warning: Could not enable debug mode");
    return NO;
}

- (BOOL)isJITAvailable {
    // Tester si on peut créer une page exécutable
    void *testPage = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
    
    if (testPage != MAP_FAILED) {
        munmap(testPage, 4096);
        NSLog(@"[DualMapping] JIT is available with MAP_JIT");
        return YES;
    }
    
    // Fallback: tester sans MAP_JIT (mode debug)
    testPage = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (testPage != MAP_FAILED) {
        // Essayer de rendre exécutable
        int result = mprotect(testPage, 4096, PROT_READ | PROT_EXEC);
        munmap(testPage, 4096);
        
        if (result == 0) {
            NSLog(@"[DualMapping] JIT is available via mprotect (debug mode)");
            return YES;
        }
    }
    
    NSLog(@"[DualMapping] JIT is NOT available");
    return NO;
}

- (DualMappedRegion*)createDualMappedRegionWithSize:(size_t)size {
    // Aligner sur la taille de page
    size_t pageSize = getpagesize();
    size = (size + pageSize - 1) & ~(pageSize - 1);
    
    DualMappedRegion *region = (DualMappedRegion*)malloc(sizeof(DualMappedRegion));
    if (!region) {
        NSLog(@"[DualMapping] Failed to allocate region structure");
        return NULL;
    }
    
    region->size = size;
    region->fd = -1;
    
    // Méthode 1: Utiliser memfd_create si disponible (iOS récent)
    #ifdef __linux__
    region->fd = memfd_create("jit_region", 0);
    #else
    // Sur iOS/macOS, utiliser shm_open
    char name[256];
    snprintf(name, sizeof(name), "/play_jit_%d_%p", getpid(), (void*)region);
    region->fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    
    if (region->fd != -1) {
        shm_unlink(name); // Supprimer immédiatement le nom
    }
    #endif
    
    if (region->fd == -1) {
        NSLog(@"[DualMapping] Failed to create shared memory: %s", strerror(errno));
        free(region);
        return NULL;
    }
    
    // Redimensionner le fichier
    if (ftruncate(region->fd, size) != 0) {
        NSLog(@"[DualMapping] Failed to resize shared memory: %s", strerror(errno));
        close(region->fd);
        free(region);
        return NULL;
    }
    
    // Premier mapping: RW (Read-Write)
    region->rwAddress = mmap(NULL, size, PROT_READ | PROT_WRITE,
                             MAP_SHARED, region->fd, 0);
    
    if (region->rwAddress == MAP_FAILED) {
        NSLog(@"[DualMapping] Failed to create RW mapping: %s", strerror(errno));
        close(region->fd);
        free(region);
        return NULL;
    }
    
    // Second mapping: RX (Read-Execute)
    // Essayer d'abord avec MAP_JIT
    region->rxAddress = mmap(NULL, size, PROT_READ | PROT_EXEC,
                             MAP_SHARED | MAP_JIT, region->fd, 0);
    
    if (region->rxAddress == MAP_FAILED) {
        // Fallback sans MAP_JIT
        region->rxAddress = mmap(NULL, size, PROT_READ | PROT_EXEC,
                                 MAP_SHARED, region->fd, 0);
    }
    
    if (region->rxAddress == MAP_FAILED) {
        NSLog(@"[DualMapping] Failed to create RX mapping: %s", strerror(errno));
        munmap(region->rwAddress, size);
        close(region->fd);
        free(region);
        return NULL;
    }
    
    NSLog(@"[DualMapping] Created dual-mapped region: RW=%p, RX=%p, size=%zu",
          region->rwAddress, region->rxAddress, size);
    
    return region;
}

- (void)freeDualMappedRegion:(DualMappedRegion*)region {
    if (!region) return;
    
    if (region->rwAddress != MAP_FAILED) {
        munmap(region->rwAddress, region->size);
    }
    
    if (region->rxAddress != MAP_FAILED) {
        munmap(region->rxAddress, region->size);
    }
    
    if (region->fd != -1) {
        close(region->fd);
    }
    
    free(region);
}

- (BOOL)writeCode:(const void*)code 
           length:(size_t)length 
         toRegion:(DualMappedRegion*)region 
           offset:(size_t)offset {
    
    if (!region || !code || offset + length > region->size) {
        return NO;
    }
    
    // Écrire dans la région RW
    memcpy((char*)region->rwAddress + offset, code, length);
    
    // Synchroniser les caches (crucial sur ARM)
    sys_icache_invalidate((char*)region->rxAddress + offset, length);
    
    return YES;
}

- (void*)getExecutablePointer:(DualMappedRegion*)region offset:(size_t)offset {
    if (!region || offset >= region->size) {
        return NULL;
    }
    
    return (char*)region->rxAddress + offset;
}

@end
