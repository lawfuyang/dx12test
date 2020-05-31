#pragma once

#include <graphic/dx12utils.h>

class GfxDescriptorHeap
{
public:
    ID3D12DescriptorHeap* Dev() const { return m_DescriptorHeap.Get(); }

    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps);

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
    static const uint32_t NumDynamicDescriptors = 1024;

    const GfxDescriptorHeap& GetInternalHeap() const { return m_InternalDescriptorHeap; }
    void Initialize();
    GfxDescriptorHeapHandle Allocate(uint32_t numHeaps, GfxDescriptorHeapHandle* out = nullptr);
    void CleanupUsedHeaps();

private:
    std::mutex m_GfxGPUDescriptorAllocatorLock;
    CircularBuffer<GfxDescriptorHeapHandle> m_FreeHeaps;
    InplaceArray<GfxDescriptorHeapHandle, NumDynamicDescriptors> m_UsedHeaps;

    GfxDescriptorHeap m_InternalDescriptorHeap;
};
#define g_GfxGPUDescriptorAllocator GfxGPUDescriptorAllocator::GetInstance()
