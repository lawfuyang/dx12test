#pragma once

#include "graphic/gfxdescriptorheap.h"

namespace D3D12MA
{
    class Allocation;
};

class GfxCommandList;
class GfxContext;

class GfxHazardTrackedResource
{
public:
    virtual ~GfxHazardTrackedResource() {}

    ID3D12Resource* GetD3D12Resource() const { return m_Resource.Get(); }

    void Transition(GfxCommandList&, D3D12_RESOURCE_STATES newState);

    void SetCurrentState(D3D12_RESOURCE_STATES newState) { m_CurrentResourceState = newState; }
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentResourceState; }

protected:
    D3D12_RESOURCE_STATES m_CurrentResourceState = D3D12_RESOURCE_STATE_COMMON;
    ComPtr<ID3D12Resource> m_Resource;
};

class GfxRenderTargetView : public GfxHazardTrackedResource
{
public:
    void Initialize(ID3D12Resource*, DXGI_FORMAT);

    GfxDescriptorHeap& GetDescriptorHeap() { return m_GfxDescriptorHeap; }

    DXGI_FORMAT GetFormat() const { return m_Format; }

private:
    GfxDescriptorHeap m_GfxDescriptorHeap;
    DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
};

class GfxVertexBuffer : public GfxHazardTrackedResource
{
public:
    ~GfxVertexBuffer();

    void Initialize(GfxContext& context, const void* vertexData, uint32_t numVertices, uint32_t vertexSize);

    D3D12_VERTEX_BUFFER_VIEW& GetBufferView() { return m_VertexBufferView; }

private:
    D3D12MA::Allocation* m_D3D12MABufferAllocation = nullptr;
    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};
};
