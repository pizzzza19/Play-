#include "JitManager.h"
#include <mach/mach.h>
#include <sys/mman.h>
#include <libkern/OSCacheControl.h>
#include <assert.h>

JitManager& JitManager::Get() {
    static JitManager instance;
    return instance;
}

bool JitManager::Initialize(size_t size) {
    // Alignement strict 16KB pour Apple Silicon
    m_size = (size + 0x3FFF) & ~0x3FFF;

    // 1. Allocation RX (Exécution)
    m_rx = mmap(nullptr, m_size, PROT_READ | PROT_EXEC, 
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);

    if (m_rx == MAP_FAILED) return false;

    // 2. Remap vers vue RW (Écriture)
    vm_address_t rwAddr = 0;
    vm_prot_t curProt, maxProt;

    kern_return_t kr = vm_remap(
        mach_task_self(),
        &rwAddr,
        m_size,
        0,
        VM_FLAGS_ANYWHERE,
        mach_task_self(),
        (vm_address_t)m_rx,
        FALSE,
        &curProt,
        &maxProt,
        VM_INHERIT_NONE
    );

    if (kr != KERN_SUCCESS) {
        munmap(m_rx, m_size);
        return false;
    }

    m_rw = (void*)rwAddr;
    mprotect(m_rw, m_size, PROT_READ | PROT_WRITE);

    return true;
}

void* JitManager::GetExecBase() const {
    return m_rx;
}

void* JitManager::GetWritePtr(void* execPtr) {
    return (void*)((uintptr_t)m_rw + ((uintptr_t)execPtr - (uintptr_t)m_rx));
}

void JitManager::Invalidate(void* execPtr, size_t size) {
    sys_icache_invalidate(execPtr, size);
}

JitManager::~JitManager() {
    if (m_rw) mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)m_rw, m_size);
    if (m_rx) munmap(m_rx, m_size);
}
