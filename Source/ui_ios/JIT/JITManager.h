#ifndef JIT_MANAGER_H
#define JIT_MANAGER_H

#include <stddef.h>
#include <stdint.h>

class JitManager {
public:
    static JitManager& Get();
    bool Initialize(size_t size);
    
    void* GetWritePtr(void* executePtr);
    void* GetExecutePtr() const { return m_rx_addr; }
    
    void Sync(void* addr, size_t size);

private:
    JitManager() = default;
    ~JitManager();

    void* m_rx_addr = nullptr;
    void* m_rw_addr = nullptr;
    size_t m_size = 0;
};

#endif
