#pragma once

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

    void SetD3D12Resource(ID3D12Resource* resource) { m_Resource = resource; }
    ID3D12Resource* GetD3D12Resource() const { return m_Resource.Get(); }

    void Transition(GfxCommandList&, D3D12_RESOURCE_STATES newState);

    void SetHazardTrackedState(D3D12_RESOURCE_STATES newState) { m_CurrentResourceState = newState; }
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_CurrentResourceState; }

protected:
    D3D12_RESOURCE_STATES m_CurrentResourceState = D3D12_RESOURCE_STATE_COMMON;
    ComPtr<ID3D12Resource> m_Resource;
};

class GfxBufferCommon
{
public:
    virtual ~GfxBufferCommon();
    void Release();

    uint32_t GetSizeInBytes() const { return m_SizeInBytes; }

protected:
    void InitializeBufferWithInitData(GfxContext& context, uint32_t uploadBufferSize, uint32_t row, uint32_t pitch, const void* initData, const char* resourceName);
    D3D12MA::Allocation* CreateHeap(GfxContext&, D3D12_HEAP_TYPE, const D3D12_RESOURCE_DESC&, D3D12_RESOURCE_STATES initialState);
    void UploadInitData(GfxContext& context, const void* dataSrc, uint32_t rowPitch, uint32_t slicePitch, ID3D12Resource* dest, ID3D12Resource* src);
    void SetInternalAllocName(D3D12MA::Allocation*, const std::string&);

    D3D12MA::Allocation* m_D3D12MABufferAllocation = nullptr;
    uint32_t             m_SizeInBytes = 0;
};

class GfxVertexBuffer : public GfxHazardTrackedResource,
                        public GfxBufferCommon
{
public:

    struct InitParams
    {
        const void*     m_InitData     = nullptr;
        uint32_t        m_NumVertices  = 0;
        uint32_t        m_VertexSize   = 0;
        const char*     m_ResourceName = "";
        D3D12_HEAP_TYPE m_HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    };

    void Initialize(GfxContext& initContext, const InitParams&);
    void Initialize(const InitParams&);

    uint32_t GetStrideInBytes() const { return m_StrideInBytes; }
    uint32_t GetNumVertices() const { return m_NumVertices; }

private:
    uint32_t m_StrideInBytes = 0;
    uint32_t m_NumVertices = 0;
};

class GfxIndexBuffer : public GfxHazardTrackedResource,
                       public GfxBufferCommon
{
public:
    struct InitParams
    {
        const void*     m_InitData     = nullptr;
        uint32_t        m_NumIndices   = 0;
        uint32_t        m_IndexSize    = 0;
        const char*     m_ResourceName = "";
        D3D12_HEAP_TYPE m_HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    };

    void Initialize(GfxContext& initContext, const InitParams&);
    void Initialize(const InitParams&);

    DXGI_FORMAT GetFormat() const { return m_Format; }
    uint32_t GetNumIndices() const { return m_NumIndices; }

private:
    DXGI_FORMAT m_Format = DXGI_FORMAT_R16_UINT;
    uint32_t m_NumIndices = 0;
};

class GfxConstantBuffer : public GfxBufferCommon
{
public:
    template <typename BufferStruct>
    void Initialize() { Initialize(sizeof(BufferStruct), BufferStruct::ms_Name); }

    void Initialize(uint32_t bufferSize, const std::string& resourceName = "");
    void Update(const void* data) const;

    GfxDescriptorHeap& GetDescriptorHeap() { return m_GfxDescriptorHeap; }

private:
    GfxDescriptorHeap m_GfxDescriptorHeap;
    void*             m_CBufferMemory = nullptr;
};

class GfxTexture : public GfxHazardTrackedResource,
                   public GfxBufferCommon
{
public:

    struct InitParams
    {
        DXGI_FORMAT           m_Format       = DXGI_FORMAT_UNKNOWN;
        uint32_t              m_Width        = 0;
        uint32_t              m_Height       = 0;
        D3D12_RESOURCE_FLAGS  m_Flags        = D3D12_RESOURCE_FLAG_NONE;
        const void*           m_InitData     = nullptr;
        const char*           m_ResourceName = "";
        D3D12_RESOURCE_STATES m_InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    };

    void Initialize(GfxContext& initContext, const InitParams&);
    void Initialize(const InitParams&);
    void InitializeForSwapChain(ComPtr<IDXGISwapChain4>& swapChain, DXGI_FORMAT, uint32_t backBufferIdx);

    GfxDescriptorHeap& GetDescriptorHeap() { return m_GfxDescriptorHeap; }

    DXGI_FORMAT GetFormat() const { return m_Format; }

private:
    GfxDescriptorHeap m_GfxDescriptorHeap;
    DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
};
