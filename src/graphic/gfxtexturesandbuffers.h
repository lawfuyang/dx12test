#pragma once

class GfxDevice;
class GfxCommandList;

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

class GfxHazardTrackedResource
{
public:
    ID3D12Resource* Dev() const { return m_Resource.Get(); }

    D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentResourceState; }

    void Set(ID3D12Resource* resource) { m_Resource = resource; }
    void Transition(GfxCommandList&, D3D12_RESOURCE_STATES newState);

private:
    D3D12_RESOURCE_STATES m_CurrentResourceState = D3D12_RESOURCE_STATE_COMMON;
    ComPtr<ID3D12Resource> m_Resource;
};

class GfxRenderTargetView
{
public:
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescHandle() const;
    
    GfxHazardTrackedResource& GetHazardTrackedResource() { return m_HazardTrackedResource; }

    void Initialize(GfxDevice& gfxDevice, ID3D12Resource*);

private:
    GfxDescriptorHeap m_DescHeap;

    GfxHazardTrackedResource m_HazardTrackedResource;
};
