#pragma once

#include <system/memorymappedfile.h>

#include <graphic/dx12utils.h>
#include <graphic/gfxrootsignature.h>
#include <graphic/gfxvertexformat.h>
#include <graphic/gfxshadermanager.h>
#include <graphic/gfxcommonstates.h>

class GfxRootSignature;
class GfxShader;

class GfxPipelineStateObject
{
public:
    void SetRootSignature(GfxRootSignature* rootSig) { m_RootSig = rootSig; }
    void SetVertexFormat(GfxVertexFormat& inputLayout) { m_VertexFormat = &inputLayout; }
    void SetVertexShader(GfxShader& shader) { m_VS = &shader; }
    void SetPixelShader(GfxShader& shader) { m_PS = &shader; }
    void SetComputeShader(GfxShader& shader) { m_CS = &shader; }
    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) { m_PrimitiveTopology = topology; }
    void SetRenderTargetFormat(uint32_t idx, DXGI_FORMAT format);
    void SetDepthStencilFormat(DXGI_FORMAT format) { m_DepthStencilFormat = format; };
    CD3DX12_BLEND_DESC& GetBlendStates() { return m_BlendStates; }
    CD3DX12_RASTERIZER_DESC& GetRasterizerStates() { return m_RasterizerStates; }
    CD3DX12_DEPTH_STENCIL_DESC1& GetDepthStencilStates() { return m_DepthStencilStates; }

private:
    GfxRootSignature*           m_RootSig = nullptr;
    GfxVertexFormat*            m_VertexFormat = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    GfxShader*                  m_VS = nullptr;
    GfxShader*                  m_PS = nullptr;
    GfxShader*                  m_CS = nullptr;
    CD3DX12_BLEND_DESC          m_BlendStates{ GfxCommonStates::Opaque };
    CD3DX12_DEPTH_STENCIL_DESC1 m_DepthStencilStates{ GfxCommonStates::DepthDefault };
    DXGI_FORMAT                 m_DepthStencilFormat = DXGI_FORMAT_UNKNOWN;
    CD3DX12_RASTERIZER_DESC     m_RasterizerStates{ GfxCommonStates::CullCounterClockwise };
    D3D12_RT_FORMAT_ARRAY       m_RenderTargets = {};
    DXGI_SAMPLE_DESC            m_SampleDescriptors = DefaultSampleDesc{};

    friend class GfxContext;
    friend class GfxPSOManager;
    friend struct std::hash<GfxPipelineStateObject>;
};

class GfxPSOManager
{
public:
    DeclareSingletonFunctions(GfxPSOManager);

    void Initialize();
    void ShutDown();

    ID3D12PipelineState* GetGraphicsPSO(const GfxPipelineStateObject&);
    ID3D12PipelineState* GetComputePSO(const GfxPipelineStateObject&);

private:
    void SavePSOToPipelineLibrary(ID3D12PipelineState* pso, const std::wstring& psoHashStr);

    ComPtr<ID3D12PipelineLibrary> m_PipelineLibrary;

    MemoryMappedFile m_MemoryMappedCacheFile;

    uint32_t m_NewPSOs = 0;
};
#define g_GfxPSOManager GfxPSOManager::GetInstance()

#include "graphic/gfxpipelinestateobject.hpp"
