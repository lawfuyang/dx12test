#pragma once

#include <pch.h>

class GfxResourceBase;

inline thread_local HRESULT tl_D3D12APIResult;
#define DX12_CALL(call)                                                                                                                   \
    {                                                                                                                                     \
        tl_D3D12APIResult = (call);                                                                                                       \
        if (FAILED(tl_D3D12APIResult)) g_Log.error("DX12 Error: return code {0:X}, call: {}", tl_D3D12APIResult, bbeTOSTRING(call));      \
        if (!SUCCEEDED(tl_D3D12APIResult)) g_Log.warn("DX12 Warning: return code {0:X}, call: {}", tl_D3D12APIResult, bbeTOSTRING(call)); \
        assert(!FAILED(tl_D3D12APIResult));                                                                                               \
    }

#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

// Helper macro for dispatch compute shader threads
#define bbeGetCSDispatchCount(domainWidth, groupWidth) (((domainWidth) + ((groupWidth) - 1)) / (groupWidth))

// sorted by highest SM
static const D3D12_FEATURE_DATA_SHADER_MODEL gs_AllD3D12ShaderModels[] =
{
    D3D_SHADER_MODEL_6_6,
    D3D_SHADER_MODEL_6_5,
    D3D_SHADER_MODEL_6_4,
    D3D_SHADER_MODEL_6_3,
    D3D_SHADER_MODEL_6_2,
    D3D_SHADER_MODEL_6_1,
    D3D_SHADER_MODEL_6_0,
    D3D_SHADER_MODEL_5_1
};

const char* GetD3DFeatureLevelName(D3D_FEATURE_LEVEL FeatureLevel);
const char* GetD3DShaderModelName(D3D_SHADER_MODEL shaderModel);
const char* GetD3DDebugName(ID3D12Object* resource);
void SetD3DDebugName(ID3D12Object* object, std::string_view name);
void SetD3DDebugParent(ID3D12Object* object, const void* parentPointer);
void* GetD3DResourceParent(ID3D12Object* resource);
D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3D12PrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
D3D12_PRIMITIVE_TOPOLOGY GetD3D12PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
uint32_t GetBitsPerPixel(DXGI_FORMAT fmt);
uint32_t GetBytesPerPixel(DXGI_FORMAT fmt);
bool IsBlockFormat(DXGI_FORMAT fmt);
void UploadToGfxResource(D3D12GraphicsCommandList* pCommandList, GfxResourceBase& destResource, D3D12_RESOURCE_STATES currentResourceState, uint32_t uploadBufferSize, uint32_t rowPitch, uint32_t slicePitch, const void* srcData);
uint32_t CountMips(uint32_t width, uint32_t height);
DXGI_FORMAT MakeSRGB(_In_ DXGI_FORMAT format);

class ScopedPixEvent
{
public:
    ScopedPixEvent(D3D12GraphicsCommandList* pCommandList, std::string_view debugName) noexcept
        : m_CommandList(pCommandList)
    {
        assert(pCommandList);
        PIXBeginEvent(pCommandList, 0, debugName.data());
    }
    ~ScopedPixEvent()
    {
        PIXEndEvent(m_CommandList);
    }

private:
    D3D12GraphicsCommandList* m_CommandList;
};
#define bbePIXEvent(cmdList, name) const ScopedPixEvent bbeUniqueVariable(scopedPixEvent) { cmdList, name }

struct ScopedD3DesourceState
{
    ScopedD3DesourceState(D3D12GraphicsCommandList* pCommandList, const GfxResourceBase& resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES tempNewState);
    ~ScopedD3DesourceState();

private:
    D3D12Resource* m_Resource;
    D3D12GraphicsCommandList* m_CommandList;
    const D3D12_RESOURCE_STATES m_OldState;
    const D3D12_RESOURCE_STATES m_TempNewState;
};
#define bbeScopedD3DResourceState(pCommandList, resource, currentState, tempNewState) const ScopedD3DesourceState bbeUniqueVariable(scopedD3DesourceState) { pCommandList, resource, currentState, tempNewState }

struct CD3D12_RENDER_TARGET_VIEW_DESC : public D3D12_RENDER_TARGET_VIEW_DESC
{
    explicit CD3D12_RENDER_TARGET_VIEW_DESC(D3D12_RTV_DIMENSION viewDimension,
                                            DXGI_FORMAT format   = DXGI_FORMAT_UNKNOWN,
                                            UINT mipSlice        = 0,   // FirstElement for BUFFER
                                            UINT firstArraySlice = 0,   // NumElements for BUFFER, FirstWSlice for TEXTURE3D
                                            UINT arraySize       = -1); // WSize for TEXTURE3D
};

struct CD3D12_SHADER_RESOURCE_VIEW_DESC : public D3D12_SHADER_RESOURCE_VIEW_DESC
{
    explicit CD3D12_SHADER_RESOURCE_VIEW_DESC(D3D12_SRV_DIMENSION viewDimension,
                                              DXGI_FORMAT format   = DXGI_FORMAT_UNKNOWN,
                                              UINT mostDetailedMip = 0,    // FirstElement for BUFFER
                                              UINT mipLevels       = -1,   // NumElements for BUFFER
                                              UINT firstArraySlice = 0,    // First2DArrayFace for TEXTURECUBEARRAY
                                              UINT arraySize       = -1,   // NumCubes for TEXTURECUBEARRAY
                                              UINT flags           = 0);   // BUFFEREX only
};

struct CD3D12_UNORDERED_ACCESS_VIEW_DESC : public D3D12_UNORDERED_ACCESS_VIEW_DESC
{
    explicit CD3D12_UNORDERED_ACCESS_VIEW_DESC(D3D12_UAV_DIMENSION viewDimension,
                                               DXGI_FORMAT format   = DXGI_FORMAT_UNKNOWN,
                                               UINT mipSlice        = 0,  // FirstElement for BUFFER
                                               UINT firstArraySlice = 0,  // NumElements for BUFFER, FirstWSlice for TEXTURE3D
                                               UINT arraySize       = -1, // WSize for TEXTURE3D
                                               UINT flags           = 0); // BUFFER only
};
