#pragma once

#include <graphic/dx12utils.h>

class GfxDescriptorHeap
{
public:
    ID3D12DescriptorHeap* Dev() const { return m_DescriptorHeap.Get(); }

    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps);
    void Release() { m_DescriptorHeap.Reset(); };

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
    static const uint32_t NbShaderVisibleDescriptors = BBE_KB(1);

    const GfxDescriptorHeap& GetInternalShaderVisibleHeap() const { return m_ShaderVisibleDescriptorHeap; }
    void Initialize();
    GfxDescriptorHeapHandle AllocateShaderVisible(uint32_t numHeaps);

private:
    GfxDescriptorHeapHandle AllocateShaderVisibleInternal(uint32_t numHeaps);

    // Shader Visible heaps
    uint32_t m_AllocationCounter = 0;
    CircularBuffer<GfxDescriptorHeapHandle> m_FreeShaderVisibleHeaps;
    GfxDescriptorHeap m_ShaderVisibleDescriptorHeap;
};
#define g_GfxGPUDescriptorAllocator GfxGPUDescriptorAllocator::GetInstance()
