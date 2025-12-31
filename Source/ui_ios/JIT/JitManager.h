#pragma once

#include <cstddef>
#include <cstdint>

class JitManager {
public:
    static JitManager& Get();

    bool Initialize(size_t size);

    void* GetExecBase() const;
    void* GetWritePtr(void* execPtr);

    void Invalidate(void* execPtr, size_t size);

private:
    JitManager() = default;
    ~JitManager();

    void* m_rx = nullptr;
    void* m_rw = nullptr;
    size_t m_size = 0;
};
