#pragma once

#include <graphic/gfx/gfxdescriptorheap.h>
#include <graphic/dx12utils.h>

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

    D3D12_RESOURCE_STATES GetResourceCurrentState() const { return m_CurrentResourceState; }
    D3D12_RESOURCE_STATES GetResourceTransitioningState() const { return m_TransitioningState; }

protected:
    D3D12_RESOURCE_STATES m_CurrentResourceState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES m_TransitioningState = D3D12_RESOURCE_STATE_COMMON;
    ID3D12Resource* m_HazardTrackedResource = nullptr;

    friend class GfxBufferCommon;
    friend class GfxContext;
};

class GfxBufferCommon
{
public:
    virtual ~GfxBufferCommon();

    ID3D12Resource* GetD3D12Resource() const { return m_D3D12MABufferAllocation->GetResource(); }

    void Release();
    static void ReleaseAllocation(D3D12MA::Allocation*);

    uint32_t GetSizeInBytes() const { return m_SizeInBytes; }

    struct HeapDesc
    {
        D3D12_HEAP_TYPE           m_HeapType            = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC       m_ResourceDesc        = {};
        D3D12_RESOURCE_STATES     m_InitialState        = D3D12_RESOURCE_STATE_COMMON;
        D3D12_CLEAR_VALUE         m_ClearValue          = {};
        D3D12MA::ALLOCATION_FLAGS m_AllocationFlags     = D3D12MA::ALLOCATION_FLAG_WITHIN_BUDGET;
        const char*               m_ResourceName        = "";
    };
    static D3D12MA::Allocation* CreateHeap(const HeapDesc&);

protected:

    void InitializeBufferWithInitData(GfxContext& context, uint32_t uploadBufferSize, uint32_t rowPitch, uint32_t slicePitch, const void* initData, const char* resourceName);
    void UploadInitData(GfxContext& context, const void* dataSrc, uint32_t rowPitch, uint32_t slicePitch, ID3D12Resource* dest, ID3D12Resource* src);

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
        std::string     m_ResourceName;
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
        std::string     m_ResourceName = "";
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
    GfxConstantBuffer(uint32_t rootIndex, uint32_t rootOffset = 0)
        : m_RootIndex(rootIndex)
        , m_RootOffset(rootOffset)
    {}

    template <typename CBStruct>
    void UploadToGPU(GfxContext& context, const CBStruct& cb) { UploadToGPU(context, (const void*)&cb, sizeof(CBStruct), CBStruct::ms_Name); }

private:
    void UploadToGPU(GfxContext& context, const void* data, uint32_t bufferSize, const char* CBName);

    uint32_t m_RootIndex;
    uint32_t m_RootOffset;
};

class GfxTexture : public GfxHazardTrackedResource,
                   public GfxBufferCommon
{
public:
    DeclareObjectModelFunctions(GfxTexture);

    enum ViewType { RTV, SRV, DSV, UAV };

    struct InitParams
    {
        DXGI_FORMAT              m_Format       = DXGI_FORMAT_UNKNOWN;
        uint32_t                 m_Width        = 0;
        uint32_t                 m_Height       = 0;
        D3D12_RESOURCE_DIMENSION m_Dimension    = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        D3D12_RESOURCE_FLAGS     m_Flags        = D3D12_RESOURCE_FLAG_NONE;
        const void*              m_InitData     = nullptr;
        std::string              m_ResourceName = "";
        D3D12_RESOURCE_STATES    m_InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        D3D12_CLEAR_VALUE        m_ClearValue   = {};
        ViewType                 m_ViewType     = SRV;
    };

    void Initialize(GfxContext& initContext, const InitParams&);
    void Initialize(const InitParams&);
    void InitializeForSwapChain(ComPtr<IDXGISwapChain4>& swapChain, DXGI_FORMAT, uint32_t backBufferIdx);

    GfxDescriptorHeap& GetDescriptorHeap() { return m_GfxDescriptorHeap; }

    DXGI_FORMAT GetFormat() const { return m_Format; }
    uint32_t GetArraySize() const { return m_ArraySize; }
    uint32_t GetNumMips() const { return m_NumMips; }

    void UpdateIMGUI();

private:
    void CreateRTV(const InitParams& initParams);
    void CreateDSV(const InitParams& initParams);
    void CreateSRV(const InitParams& initParams);

    GfxDescriptorHeap m_GfxDescriptorHeap;
    DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;
    uint32_t m_ArraySize = 1;
    uint32_t m_NumMips = 1;
};
