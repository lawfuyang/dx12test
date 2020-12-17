#pragma once

class GfxMemoryAllocator
{
    DeclareSingletonFunctions(GfxMemoryAllocator);

public:
    void Initialize();

    D3D12MA::Allocator& Dev() { assert(m_D3D12MemoryAllocator); return *m_D3D12MemoryAllocator; }

    D3D12MA::Allocation* AllocateStatic(const CD3DX12_RESOURCE_DESC1&, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE*);
    D3D12MA::Allocation* AllocateVolatile(D3D12_HEAP_TYPE, const CD3DX12_RESOURCE_DESC1&);

    void ShutDown();
    void ReleaseStatic(D3D12MA::Allocation*);
    void GarbageCollect();

private:
    D3D12MA::Allocation* AllocateInternal(D3D12_HEAP_TYPE, const CD3DX12_RESOURCE_DESC1&, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE*);

    D3D12MA::Allocator* m_D3D12MemoryAllocator = nullptr;

    std::mutex m_StaticAllocationsLck;
    FlatSet<D3D12MA::Allocation*> m_StaticAllocations;

    std::mutex m_VolatileAllocationsLck;
    InplaceArray<D3D12MA::Allocation*, 128> m_VolatileAllocations;
};
#define g_GfxMemoryAllocator GfxMemoryAllocator::GetInstance()
