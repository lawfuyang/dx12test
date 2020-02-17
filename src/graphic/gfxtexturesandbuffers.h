#pragma once

#include "extern/D3D12MemoryAllocator/src/D3D12MemAlloc.h"

#include "graphic/gfxdescriptorheap.h"
#include "graphic/dx12utils.h"

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

class GfxBufferCommon
{
public:
    virtual ~GfxBufferCommon();

    uint32_t GetSizeInBytes() const { return m_SizeInBytes; }

protected:
    void CreateDefaultHeap(GfxContext& context, const CD3DX12_RESOURCE_DESC& resourceDesc, ComPtr<ID3D12Resource>& resource);
    ID3D12Resource* CreateUploadHeapForDataInit(GfxContext& context, const CD3DX12_RESOURCE_DESC& resourceDesc);
    void UploadInitData(GfxContext& context, const void* dataSrc, uint32_t rowPitch, uint32_t slicePitch, ID3D12Resource* dest, ID3D12Resource* src);

    D3D12MA::Allocation* m_D3D12MABufferAllocation = nullptr;
    uint32_t             m_SizeInBytes = 0;
};

class GfxVertexBuffer : public GfxHazardTrackedResource,
                        public GfxBufferCommon
{
public:
    D3D12MA::Allocation* Initialize(GfxContext& context, const void* vList, uint32_t numVertices, uint32_t vertexSize);

    uint32_t GetStrideInBytes() const { return m_StrideInBytes; }

private:
    uint32_t m_StrideInBytes = 0;
};

class GfxIndexBuffer : public GfxHazardTrackedResource,
                       public GfxBufferCommon
{
public:
    D3D12MA::Allocation* Initialize(GfxContext& context, const void* iList, uint32_t numIndices, uint32_t indexSize);

    DXGI_FORMAT GetFormat() const { return m_Format; }

private:
    DXGI_FORMAT m_Format = DXGI_FORMAT_R16_UINT;
};

class GfxConstantBuffer : public GfxBufferCommon
{
public:
    D3D12MA::Allocation* Initialize();

    GfxDescriptorHeap& GetDescriptorHeap() { return m_GfxDescriptorHeap; }

private:
    GfxDescriptorHeap m_GfxDescriptorHeap;
};
