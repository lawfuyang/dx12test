#include "graphic/dx12utils.h"

#include "system/utils.h"
#include "system/logger.h"

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

    default: return "D3D_SHADER_MODEL_6_5+";
    }
}

const std::string GetD3DDebugName(ID3D12Object* object)
{
    UINT size = MAX_PATH;
    char name[MAX_PATH] = {};
    DX12_CALL(object->GetPrivateData(WKPDID_D3DDebugObjectName, &size, &name));
    return name;
}

void SetD3DDebugName(ID3D12Object* object, const std::string& name)
{
    object->SetName(MakeWStrFromStr(name).c_str());
    object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.c_str());
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
        break;
    case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
    case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
    case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        break;
    case D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST:
    case D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        break;
#ifdef POP_PLATFORM_SUPPORTS_RECT_PRIMITIVE
    case D3D_PRIMITIVE_TOPOLOGY_RECTLIST:
        return D3D12XBOX_PRIMITIVE_TOPOLOGY_TYPE_RECT;
        break;
#endif
    default:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        break;
    }
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
