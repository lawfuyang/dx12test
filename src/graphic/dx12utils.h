#pragma once

//------------------------------------------------------------------------------------------------------------------------------------------------------
#define DX12_CALL(call)                                                                                            \
    {                                                                                                              \
        const HRESULT result = call;                                                                               \
        if (FAILED(result)) g_Log.error("DX12 Error: return code {0:X}, call: {}", result, bbeTOSTRING(call));     \
        if (!SUCCEEDED(result)) g_Log.warn("DX12 Warning: return code {0:X}, call: {}", result, bbeTOSTRING(call));\
        assert(!FAILED(result));                                                                                   \
    }

#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

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

class ScopedPixEvent
{
public:
    ScopedPixEvent(ID3D12GraphicsCommandList* pCommandList) noexcept
        : m_CommandList(pCommandList)
    {
        assert(pCommandList);
        PIXBeginEvent(pCommandList, 0, GetD3DDebugName(pCommandList));
    }
    ~ScopedPixEvent()
    {
        PIXEndEvent(m_CommandList);
    }

private:
    ID3D12GraphicsCommandList* m_CommandList;
};

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
