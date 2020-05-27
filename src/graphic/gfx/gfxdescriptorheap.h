#pragma once

class GfxDescriptorHeap
{
public:
    ID3D12DescriptorHeap* Dev() const { return m_DescriptorHeap.Get(); }

    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags);

private:
    ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
};
