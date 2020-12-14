#include <graphic/dx12utils.h>
#include <graphic/pch.h>

ScopedD3DesourceState::ScopedD3DesourceState(D3D12GraphicsCommandList* pCommandList, const GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES tempNewState)
    : m_CommandList(pCommandList)
    , m_Resource(resource.GetD3D12Resource())
    , m_OldState(resource.m_CurrentResourceState)
    , m_TempNewState(tempNewState)
{
    const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Resource, m_OldState, m_TempNewState);
    m_CommandList->ResourceBarrier(1, &barrier);
}

ScopedD3DesourceState::~ScopedD3DesourceState()
{
    const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Resource, m_TempNewState, m_OldState);
    m_CommandList->ResourceBarrier(1, &barrier);
}

// {4AA579AB-315B-4D6B-BE23-17FFD8402316}
static const GUID GUID_ParentPointer = { 0x4aa579ab, 0x315b, 0x4d6b,{ 0xbe, 0x23, 0x17, 0xff, 0xd8, 0x40, 0x23, 0x16 } };

const char* GetD3DFeatureLevelName(D3D_FEATURE_LEVEL FeatureLevel)
{
    switch (FeatureLevel)
    {
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_1_0_CORE);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_9_1);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_9_2);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_9_3);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_10_0);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_10_1);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_11_0);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_11_1);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_12_0);
        bbeSimpleSwitchCaseString(D3D_FEATURE_LEVEL_12_1);

    default: return "D3D_FEATURE_LEVEL_12_1+";
    }
}

const char* GetD3DShaderModelName(D3D_SHADER_MODEL shaderModel)
{
    switch (shaderModel)
    {
        bbeSimpleSwitchCaseString(D3D_SHADER_MODEL_5_1);
        bbeSimpleSwitchCaseString(D3D_SHADER_MODEL_6_0);
        bbeSimpleSwitchCaseString(D3D_SHADER_MODEL_6_1);
        bbeSimpleSwitchCaseString(D3D_SHADER_MODEL_6_2);
        bbeSimpleSwitchCaseString(D3D_SHADER_MODEL_6_3);
        bbeSimpleSwitchCaseString(D3D_SHADER_MODEL_6_4);
        bbeSimpleSwitchCaseString(D3D_SHADER_MODEL_6_5);
        bbeSimpleSwitchCaseString(D3D_SHADER_MODEL_6_6);

    default: return "D3D_SHADER_MODEL_6_6+";
    }
}

const char* GetD3DDebugName(ID3D12Object* object)
{
    UINT size = MAX_PATH;
    thread_local char tl_Name[MAX_PATH]{};
    DX12_CALL(object->GetPrivateData(WKPDID_D3DDebugObjectName, &size, &tl_Name));
    return tl_Name;
}

void SetD3DDebugName(ID3D12Object* object, std::string_view name)
{
    object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.data());
}

void SetD3DDebugParent(ID3D12Object* object, const void* parentPointer)
{
    object->SetPrivateData(GUID_ParentPointer, sizeof(parentPointer), parentPointer);
}

void* GetD3DResourceParent(ID3D12Object* resource)
{
    UINT size = sizeof(void*);
    void* parentPointer = nullptr;
    resource->GetPrivateData(GUID_ParentPointer, &size, &parentPointer);
    return parentPointer;
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3D12PrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology)
{
    switch (primitiveTopology)
    {
    case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

    case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    case D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }

    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
}

D3D12_PRIMITIVE_TOPOLOGY GetD3D12PrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
    switch (topologyType)
    {
    case D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

    case D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;

    case D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }

    return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

uint32_t GetBitsPerPixel(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 8;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

    case DXGI_FORMAT_V408:
        return 24;

    case DXGI_FORMAT_P208:
    case DXGI_FORMAT_V208:
        return 16;

#endif // #if (_WIN32_WINNT >= _WIN32_WINNT_WIN10)

    case DXGI_FORMAT_UNKNOWN:
    case DXGI_FORMAT_FORCE_UINT:
    default:
        return 0;
    }
}

uint32_t GetBytesPerPixel(DXGI_FORMAT fmt)
{
    return std::max(1U, GetBitsPerPixel(fmt) / 8);
}

bool IsBlockFormat(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return true;
    }

    return false;
}

void UploadToGfxResource(D3D12GraphicsCommandList* pCommandList, GfxHazardTrackedResource& destResource, uint32_t uploadBufferSize, uint32_t rowPitch, uint32_t slicePitch, const void* srcData)
{
    bbePIXEvent(pCommandList, "UpdateSubresources");

    assert(pCommandList);
    assert(destResource.GetD3D12Resource());
    assert(uploadBufferSize > 0);
    assert(rowPitch > 0);
    assert(slicePitch > 0);
    assert(srcData);

    D3D12MA::Allocation* uploadHeapAlloc = g_GfxMemoryAllocator.Allocate(D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC1::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
    SetD3DDebugName(destResource.GetD3D12Resource(), "UpdateSubresources");

    bbeScopedD3DResourceState(pCommandList, destResource, D3D12_RESOURCE_STATE_COPY_DEST);

    // upload init data via CopyTextureRegion/CopyBufferRegion
    const UINT MaxSubresources = 1;
    const UINT64 IntermediateOffset = 0;
    const UINT FirstSubresource = 0;
    const UINT NumSubresources = 1;
    const D3D12_SUBRESOURCE_DATA data{ srcData, rowPitch, slicePitch };
    const UINT64 r = UpdateSubresources<MaxSubresources>(pCommandList, destResource.GetD3D12Resource(), uploadHeapAlloc->GetResource(), IntermediateOffset, FirstSubresource, NumSubresources, &data);
    assert(r);

    // TODO: Implement a gfx garbage collector
    g_GfxManager.AddGraphicCommand([uploadHeapAlloc]() { g_GfxMemoryAllocator.Release(uploadHeapAlloc); });
}

CD3D12_RENDER_TARGET_VIEW_DESC::CD3D12_RENDER_TARGET_VIEW_DESC(D3D12_RTV_DIMENSION viewDimension,
                                                               DXGI_FORMAT format,
                                                               UINT mipSlice,
                                                               UINT firstArraySlice,
                                                               UINT arraySize)
{
    Format = format;
    ViewDimension = viewDimension;
    switch (viewDimension)
    {
    case D3D12_RTV_DIMENSION_BUFFER:
        Buffer.FirstElement = mipSlice;
        Buffer.NumElements = firstArraySlice;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE1D:
        Texture1D.MipSlice = mipSlice;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MipSlice = mipSlice;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE2D:
        Texture2D.MipSlice = mipSlice;
        Texture2D.PlaneSlice = 0;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MipSlice = mipSlice;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.PlaneSlice = 0;
        Texture2DArray.ArraySize = arraySize;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE2DMS:
        break;
    case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
        Texture2DMSArray.FirstArraySlice = firstArraySlice;
        Texture2DMSArray.ArraySize = arraySize;
        break;
    case D3D12_RTV_DIMENSION_TEXTURE3D:
        Texture3D.MipSlice = mipSlice;
        Texture3D.FirstWSlice = firstArraySlice;
        Texture3D.WSize = arraySize;
        break;
    default: break;
    }
}

CD3D12_SHADER_RESOURCE_VIEW_DESC::CD3D12_SHADER_RESOURCE_VIEW_DESC(D3D12_SRV_DIMENSION viewDimension,
                                                                   DXGI_FORMAT format,
                                                                   UINT mostDetailedMip,
                                                                   UINT mipLevels,
                                                                   UINT firstArraySlice,
                                                                   UINT arraySize,
                                                                   UINT flags)
{
    Format = format;
    ViewDimension = viewDimension;
    Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (viewDimension)
    {
    case D3D12_SRV_DIMENSION_BUFFER:
        Buffer.FirstElement = mostDetailedMip;
        Buffer.NumElements = mipLevels;
        Buffer.StructureByteStride = 0;
        Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE1D:
        Texture1D.MostDetailedMip = mostDetailedMip;
        Texture1D.MipLevels = mipLevels;
        Texture1D.ResourceMinLODClamp = float(mostDetailedMip);
        break;
    case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MostDetailedMip = mostDetailedMip;
        Texture1DArray.MipLevels = mipLevels;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        Texture1DArray.ResourceMinLODClamp = float(mostDetailedMip);
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2D:
        Texture2D.MostDetailedMip = mostDetailedMip;
        Texture2D.MipLevels = mipLevels;
        Texture2D.PlaneSlice = 0;
        Texture2D.ResourceMinLODClamp = float(mostDetailedMip);
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MostDetailedMip = mostDetailedMip;
        Texture2DArray.MipLevels = mipLevels;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.ArraySize = arraySize;
        Texture2DArray.PlaneSlice = 0;
        Texture2DArray.ResourceMinLODClamp = float(mostDetailedMip);
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2DMS:
        break;
    case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
        Texture2DMSArray.FirstArraySlice = firstArraySlice;
        Texture2DMSArray.ArraySize = arraySize;
        break;
    case D3D12_SRV_DIMENSION_TEXTURE3D:
        Texture3D.MostDetailedMip = mostDetailedMip;
        Texture3D.MipLevels = mipLevels;
        Texture3D.ResourceMinLODClamp = float(mostDetailedMip);
        break;
    case D3D12_SRV_DIMENSION_TEXTURECUBE:
        TextureCube.MostDetailedMip = mostDetailedMip;
        TextureCube.MipLevels = mipLevels;
        TextureCube.ResourceMinLODClamp = float(mostDetailedMip);
        break;
    case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
        TextureCubeArray.MostDetailedMip = mostDetailedMip;
        TextureCubeArray.MipLevels = mipLevels;
        TextureCubeArray.First2DArrayFace = firstArraySlice;
        TextureCubeArray.NumCubes = arraySize;
        TextureCubeArray.ResourceMinLODClamp = float(mostDetailedMip);
        break;
    default: break;
    }
}

CD3D12_UNORDERED_ACCESS_VIEW_DESC::CD3D12_UNORDERED_ACCESS_VIEW_DESC(D3D12_UAV_DIMENSION viewDimension,
                                                                     DXGI_FORMAT format,
                                                                     UINT mipSlice,
                                                                     UINT firstArraySlice,
                                                                     UINT arraySize,
                                                                     UINT flags)
{
    Format = format;
    ViewDimension = viewDimension;
    switch (viewDimension)
    {
    case D3D12_UAV_DIMENSION_BUFFER:
        Buffer.FirstElement = mipSlice;
        Buffer.NumElements = firstArraySlice;
        Buffer.CounterOffsetInBytes = 0;
        Buffer.Flags = D3D12_BUFFER_UAV_FLAGS(flags);
        break;
    case D3D12_UAV_DIMENSION_TEXTURE1D:
        Texture1D.MipSlice = mipSlice;
        break;
    case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
        Texture1DArray.MipSlice = mipSlice;
        Texture1DArray.FirstArraySlice = firstArraySlice;
        Texture1DArray.ArraySize = arraySize;
        break;
    case D3D12_UAV_DIMENSION_TEXTURE2D:
        Texture2D.MipSlice = mipSlice;
        Texture2D.PlaneSlice = 0;
        break;
    case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
        Texture2DArray.MipSlice = mipSlice;
        Texture2DArray.FirstArraySlice = firstArraySlice;
        Texture2DArray.ArraySize = arraySize;
        Texture2DArray.PlaneSlice = 0;
        break;
    case D3D12_UAV_DIMENSION_TEXTURE3D:
        Texture3D.MipSlice = mipSlice;
        Texture3D.FirstWSlice = firstArraySlice;
        Texture3D.WSize = arraySize;
        break;
    default: break;
    }
}
