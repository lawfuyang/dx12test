#pragma once

#include "graphic/gfxdescriptorheap.h"

class GfxCommandList;

class GfxHazardTrackedResource
{
public:
    ID3D12Resource* Dev() const { return m_Resource.Get(); }

    void Set(ID3D12Resource* resource) { m_Resource = resource; }
    void Transition(GfxCommandList&, D3D12_RESOURCE_STATES newState);

    D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentResourceState; }

private:
    D3D12_RESOURCE_STATES m_CurrentResourceState = D3D12_RESOURCE_STATE_COMMON;
    ComPtr<ID3D12Resource> m_Resource;
};

class GfxResourceView
{
public:
    virtual ~GfxResourceView() {}

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescHandle() const { return m_DescHeapHandle.m_CPUHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescHandle() const { return m_DescHeapHandle.m_GPUHandle; }

    GfxHazardTrackedResource& GetHazardTrackedResource() { return m_HazardTrackedResource; }

protected:
    void InitializeCommon(ID3D12Resource*, D3D12_DESCRIPTOR_HEAP_TYPE, D3D12_DESCRIPTOR_HEAP_FLAGS);

    GfxDescriptorHeapHandle m_DescHeapHandle;

    GfxHazardTrackedResource m_HazardTrackedResource;
};

class GfxRenderTargetView : public GfxResourceView
{
public:
    void Initialize(ID3D12Resource*);
};

class GfxShaderResourceView : public GfxResourceView
{
public:
    void Initialize(ID3D12Resource*);
};
