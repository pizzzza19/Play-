#include "JitMemory.h"
#include "JitManager.h"
#include "JitConfig.h"

static uint8_t* g_execBase = nullptr;
static size_t g_offset = 0;

void JitMemory::Initialize() {
#if PLAY_IOS_JIT
    if (JitManager::Get().Initialize(PLAY_JIT_REGION_SIZE)) {
        g_execBase = (uint8_t*)JitManager::Get().GetExecBase();
    }
#endif
}

uint8_t* JitMemory::Allocate(size_t size) {
    if (!g_execBase) return nullptr;
    uint8_t* ptr = g_execBase + g_offset;
    g_offset += size;
    return ptr;
}

uint8_t* JitMemory::GetWritePtr(uint8_t* execPtr) {
#if PLAY_IOS_JIT
    return (uint8_t*)JitManager::Get().GetWritePtr(execPtr);
#else
    return execPtr;
#endif
}

void JitMemory::Flush(uint8_t* execPtr, size_t size) {
#if PLAY_IOS_JIT
    JitManager::Get().Invalidate(execPtr, size);
#endif
}
