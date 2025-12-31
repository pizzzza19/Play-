#ifndef JIT_MEMORY_H
#define JIT_MEMORY_H

#include <stdint.h>
#include <stddef.h>

namespace JitMemory {
    void Initialize();
    uint8_t* Allocate(size_t size);
    uint8_t* GetWritePtr(uint8_t* execPtr);
    void Flush(uint8_t* execPtr, size_t size);
}

#endif
