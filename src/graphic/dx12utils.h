#pragma once

#include <string>
#include <cassert>

//------------------------------------------------------------------------------------------------------------------------------------------------------
#define DX12_CALL(call)                                                                               \
    {                                                                                                 \
        HRESULT result = call;                                                                        \
        if (FAILED(result)) g_Log.error("DX12 Error: return code 0x%X, call: {}", result, #call);     \
        if (!SUCCEEDED(result)) g_Log.warn("DX12 Warning: return code 0x%X, call: {}", result, #call);\
        assert(!FAILED(result));                                                                      \
    }

//------------------------------------------------------------------------------------------------------------------------------------------------------
const char* GetD3DFeatureLevelName(D3D_FEATURE_LEVEL FeatureLevel);

//------------------------------------------------------------------------------------------------------------------------------------------------------
const char* GetD3DShaderModelName(D3D_SHADER_MODEL shaderModel);

//------------------------------------------------------------------------------------------------------------------------------------------------------
const std::string GetD3DDebugName(ID3D12Object* resource);

//------------------------------------------------------------------------------------------------------------------------------------------------------
void SetD3DDebugName(ID3D12Object* object, const std::string& name);

//------------------------------------------------------------------------------------------------------------------------------------------------------
void SetD3DDebugParent(ID3D12Object* object, const void* parentPointer);

//------------------------------------------------------------------------------------------------------------------------------------------------------
void* GetD3DResourceParent(ID3D12Object* resource);
