#include "graphic/dx12utils.h"

#include "system/utils.h"
#include "system/logger.h"

// {4AA579AB-315B-4D6B-BE23-17FFD8402316}
static const GUID GUID_ParentPointer = { 0x4aa579ab, 0x315b, 0x4d6b,{ 0xbe, 0x23, 0x17, 0xff, 0xd8, 0x40, 0x23, 0x16 } };

//------------------------------------------------------------------------------------------------------------------------------------------------------
const char* GetD3DFeatureLevelName(D3D_FEATURE_LEVEL FeatureLevel)
{
    switch (FeatureLevel)
    {
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

//------------------------------------------------------------------------------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------------------------------------------------------------------------------
const std::string GetD3DDebugName(ID3D12Object* object)
{
    UINT size = MAX_PATH;
    char name[MAX_PATH] = {};
    DX12_CALL(object->GetPrivateData(WKPDID_D3DDebugObjectName, &size, &name));
    return name;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void SetD3DDebugName(ID3D12Object* object, const std::string& name)
{
    object->SetName(utf8_decode(name).c_str());
    object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(), name.c_str());
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void SetD3DDebugParent(ID3D12Object* object, const void* parentPointer)
{
    object->SetPrivateData(GUID_ParentPointer, sizeof(parentPointer), parentPointer);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void* GetD3DResourceParent(ID3D12Object* resource)
{
    UINT size = sizeof(void*);
    void* parentPointer = nullptr;
    resource->GetPrivateData(GUID_ParentPointer, &size, &parentPointer);
    return parentPointer;
}
