#pragma once

#include <graphic/dx12utils.h>

class GfxDescriptorHeap
{
public:
    ID3D12DescriptorHeap* Dev() const { return m_DescriptorHeap.Get(); }

    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps);

    bool IsShaderVisible() const { return m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr != 0; }

private:
    ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
};

struct GfxDescriptorHeapHandle
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;

    void Offset(INT offsetInDescriptors, UINT descriptorIncrementSize)
    {
        m_CPUHandle.Offset(offsetInDescriptors, descriptorIncrementSize);
        m_GPUHandle.Offset(offsetInDescriptors, descriptorIncrementSize);
    }
};

// No D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER support! 100% Static Samplers usage!
class GfxGPUDescriptorAllocator
{
    DeclareSingletonFunctions(GfxGPUDescriptorAllocator);

public:
    static const uint32_t NbDescriptors = 1024;
    static const uint32_t NbDescHeapContexts = 2;

    const GfxDescriptorHeap& GetInternalHeap(D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags) const { return m_InternalDescriptorHeap[heapFlags]; }
    void Initialize();
    GfxDescriptorHeapHandle AllocateVolatile(D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps, GfxDescriptorHeapHandle* out = nullptr);
    void GarbageCollect();

private:
    GfxDescriptorHeapHandle AllocateVolatileInternal(D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps, GfxDescriptorHeapHandle* out);

    std::mutex m_GfxGPUDescriptorAllocatorLock[NbDescHeapContexts];
    uint32_t m_AllocationCounter[NbDescHeapContexts]{};
    CircularBuffer<GfxDescriptorHeapHandle> m_FreeHeaps[NbDescHeapContexts];
    InplaceArray<GfxDescriptorHeapHandle, NbDescriptors> m_UsedHeaps[NbDescHeapContexts];

    GfxDescriptorHeap m_InternalDescriptorHeap[NbDescHeapContexts];
};
#define g_GfxGPUDescriptorAllocator GfxGPUDescriptorAllocator::GetInstance()
