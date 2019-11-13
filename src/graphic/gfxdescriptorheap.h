#pragma once

class GfxDevice;

class GfxDescriptorHeap
{
public:
    ID3D12DescriptorHeap* Dev() const { return m_DescriptorHeap.Get(); }

    void Initialize(GfxDevice& gfxDevice, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
    uint32_t GetDescSize() const { return m_DescriptorSize; }

private:
    ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
    uint32_t m_DescriptorSize = 0;
};
