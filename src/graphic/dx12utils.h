#pragma once

#define USE_PIX
#include <extern/pix/pix3.h>

#include <string>
#include <assert.h>

//------------------------------------------------------------------------------------------------------------------------------------------------------
#define DX12_CALL(call)                                                                                            \
    {                                                                                                              \
        const HRESULT result = call;                                                                               \
        if (FAILED(result)) g_Log.error("DX12 Error: return code {0:X}, call: {}", result, bbeTOSTRING(call));     \
        if (!SUCCEEDED(result)) g_Log.warn("DX12 Warning: return code {0:X}, call: {}", result, bbeTOSTRING(call));\
        assert(!FAILED(result));                                                                                   \
    }

const char* GetD3DFeatureLevelName(D3D_FEATURE_LEVEL FeatureLevel);
const char* GetD3DShaderModelName(D3D_SHADER_MODEL shaderModel);
const std::string GetD3DDebugName(ID3D12Object* resource);
void SetD3DDebugName(ID3D12Object* object, const std::string& name);
void SetD3DDebugParent(ID3D12Object* object, const void* parentPointer);
void* GetD3DResourceParent(ID3D12Object* resource);
D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3D12PrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY primitiveTopology);
uint32_t GetBitsPerPixel(DXGI_FORMAT fmt);
uint32_t GetBytesPerPixel(DXGI_FORMAT fmt);

class ScopedPixEvent
{
public:
    ScopedPixEvent(ID3D12GraphicsCommandList* pCommandList) noexcept
        : m_CommandList(pCommandList)
    {
        if (g_CommandLineOptions.m_PIXCapture)
        {
            assert(pCommandList);
            PIXBeginEvent(pCommandList, 0, GetD3DDebugName(pCommandList).c_str());
        }
    }
    ~ScopedPixEvent()
    {
        if (g_CommandLineOptions.m_PIXCapture)
            PIXEndEvent(m_CommandList);
    }

private:
    ID3D12GraphicsCommandList* m_CommandList;
};
