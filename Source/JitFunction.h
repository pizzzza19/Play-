#pragma once

class CMemoryFunction;

// Preserve the host ABI once around the complete linked JIT block chain.
void ExecuteJitFunction(CMemoryFunction&, void*);
