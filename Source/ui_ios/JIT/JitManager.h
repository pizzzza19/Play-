#ifndef JIT_MANAGER_H
#define JIT_MANAGER_H

#include <stddef.h>
#include <stdint.h>

class JitManager {
public:
    static JitManager& Get();
    bool Initialize(size_t size);
    void* GetWritePtr(void* execPtr);
    void* GetExecBase() const;
    void Invalidate(void* execPtr, size_t size);

private:
    JitManager() = default;
    ~JitManager();
    void* m_rx = nullptr;
    void* m_rw = nullptr;
    size_t m_size = 0;
};

#endif
