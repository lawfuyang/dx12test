#include "graphic/dx12utils.h"

// {4AA579AB-315B-4D6B-BE23-17FFD8402316}
static const GUID GUID_ParentPointer = { 0x4aa579ab, 0x315b, 0x4d6b,{ 0xbe, 0x23, 0x17, 0xff, 0xd8, 0x40, 0x23, 0x16 } };

//------------------------------------------------------------------------------------------------------------------------------------------------------
const std::string GetD3DFeatureLevelName(D3D_FEATURE_LEVEL FeatureLevel)
{
    switch (FeatureLevel)
    {
    case D3D_FEATURE_LEVEL_9_1: return "9.1";
    case D3D_FEATURE_LEVEL_9_2: return "9.2";
    case D3D_FEATURE_LEVEL_9_3: return "9.3";
    case D3D_FEATURE_LEVEL_10_0: return "10.0";
    case D3D_FEATURE_LEVEL_10_1: return "10.1";
    case D3D_FEATURE_LEVEL_11_0: return "11.0";
    case D3D_FEATURE_LEVEL_11_1: return "11.1";
    case D3D_FEATURE_LEVEL_12_0: return "12.0";
    case D3D_FEATURE_LEVEL_12_1: return "12.1";

    default: return "12.1+";
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
const std::string GetD3D12ResourceName(ID3D12Resource* resource)
{
    UINT size = MAX_PATH;
    char name[MAX_PATH] = {};
    DX12_CALL(resource->GetPrivateData(WKPDID_D3DDebugObjectName, &size, &name));
    return name;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void SetDebugName(ID3D12Object* object, const std::string& name)
{
    object->SetName(utf8_decode(name).c_str());
    object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.c_str());
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void SetDebugParent(ID3D12Object* object, const void* parentPointer)
{
    object->SetPrivateData(GUID_ParentPointer, sizeof(parentPointer), parentPointer);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void* GetD3D12ResourceParent(ID3D12Object* resource)
{
    UINT size = sizeof(void*);
    void* parentPointer = nullptr;
    resource->GetPrivateData(GUID_ParentPointer, &size, &parentPointer);
    return parentPointer;
}
