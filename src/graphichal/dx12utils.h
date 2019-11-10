#pragma once

//------------------------------------------------------------------------------------------------------------------------------------------------------
#define DX12_CALL(call)                                                                         \
    {                                                                                           \
        HRESULT result = call;                                                                  \
        bbeError(!FAILED(result), "DX12 Error: return code 0x%X, call: %s", result, #call);     \
        bbeWarning(SUCCEEDED(result), "DX12 Error: return code 0x%X, call: %s", result, #call); \
    }

#if 0

//------------------------------------------------------------------------------------------------------------------------------------------------------
const std::string GetD3D12ResourceName(ID3D12Resource* resource)
{
    static const UINT size = MAX_PATH;
    char name[size] = {};
    DX12_CALL(resource->GetPrivateData(WKPDID_D3DDebugObjectName, &size, &name));
    return std::string{ name };
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void SetDebugName(ID3D12Object* object, const char* name)
{
    WChar wResourceName[MAX_PATH];
    U32 size = U32(strlen(name)) + 1;
    FUSION_ASSERT(size < MAX_PATH);
    mbstowcs(wResourceName, name, size);
    object->SetName(wResourceName);
    object->SetPrivateData(WKPDID_D3DDebugObjectName, (U32)size, name);
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

#endif
