#pragma once

#include "graphic/gfxdescriptorheap.h"

class GfxDevice;

class GfxRenderTargetView
{
public:
    ID3D12Resource* Dev() const { return m_Resource.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescHandle() const;
    
    D3D12_RESOURCE_STATES GetCurrentResourceState() const { return m_CurrentResourceState; }
    void SetCurrentResourceState(D3D12_RESOURCE_STATES state) { m_CurrentResourceState = state; }

    void Initialize(GfxDevice& gfxDevice, ID3D12Resource*);

private:
    GfxDescriptorHeap m_DescHeap;
    D3D12_RESOURCE_STATES m_CurrentResourceState;

    ComPtr<ID3D12Resource> m_Resource;
};
