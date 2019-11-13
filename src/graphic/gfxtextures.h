#pragma once

#include "graphic/gfxdescriptorheap.h"

class GfxDevice;

class GfxRenderTargetView
{
public:
    ID3D12Resource* Dev() const { return m_Resource.Get(); }

    void Initialize(GfxDevice& gfxDevice, ID3D12Resource*);

private:
    GfxDescriptorHeap m_DescHeap;

    ComPtr<ID3D12Resource> m_Resource;
};
