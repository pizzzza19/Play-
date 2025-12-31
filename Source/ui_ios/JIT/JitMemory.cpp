#include "JitMemory.h"
#include "JitManager.h"

static uint8_t* g_execBase = nullptr;
static size_t g_offset = 0;

void JitMemory::Initialize() {
    if (JitManager::Get().Initialize(64 * 1024 * 1024)) {
        g_execBase = (uint8_t*)JitManager::Get().GetExecBase();
    }
}

uint8_t* JitMemory::Allocate(size_t size) {
    if (!g_execBase) return nullptr;
    uint8_t* ptr = g_execBase + g_offset;
    g_offset += size;
    return ptr;
}

uint8_t* JitMemory::GetWritePtr(uint8_t* execPtr) {
    return (uint8_t*)JitManager::Get().GetWritePtr(execPtr);
}

void JitMemory::Flush(uint8_t* execPtr, size_t size) {
    JitManager::Get().Invalidate(execPtr, size);
}
