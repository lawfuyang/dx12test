#pragma once

#include "system/memorymappedfile.h"

#include "graphic/gfxrootsignature.h"
#include "graphic/gfxvertexformat.h"
#include "graphic/gfxshadermanager.h"

class GfxRootSignature;
class GfxShader;

class GfxPipelineStateObject
{
public:
    void SetRootSignature(GfxRootSignature* rootSig) { m_RootSig = rootSig; }
    void SetVertexInputLayout(GfxVertexFormat& inputLayout) { m_VertexFormat = &inputLayout; }
    void SetVertexShader(GfxShader& shader) { m_VS = &shader; }
    void SetPixelShader(GfxShader& shader) { m_PS = &shader; }
    void SetComputeShader(GfxShader& shader) { m_CS = &shader; }
    void SetDepthEnable(bool b) { m_DepthStencilStates.DepthEnable = b; }
    void SetStencilEnable(bool b) { m_DepthStencilStates.StencilEnable = b; }
    void SetFillMode(D3D12_FILL_MODE fillMode) { m_RasterizerStates.FillMode = fillMode; }
    void SetCullMode(D3D12_CULL_MODE cullMode) { m_RasterizerStates.CullMode = cullMode; }
    void SetFrontCounterClockwise(bool b) { m_RasterizerStates.FrontCounterClockwise = b; }
    void SetDepthBias(uint32_t depthBias) { m_RasterizerStates.DepthBias = depthBias; }
    void SetDepthBiasClamp(float depthBiasClamp) { m_RasterizerStates.DepthBiasClamp = depthBiasClamp; }
    void SetSlopeScaledDepthBias(float slopeScaledDepthBias) { m_RasterizerStates.SlopeScaledDepthBias = slopeScaledDepthBias; }
    void SetDepthClipEnable(bool depthClipEnable) { m_RasterizerStates.DepthClipEnable = depthClipEnable; }
    void SetMultisampleEnable(bool multisampleEnable) { m_RasterizerStates.MultisampleEnable = multisampleEnable; }
    void SetAntialiasedLineEnable(bool antialiasedLineEnable) { m_RasterizerStates.AntialiasedLineEnable = antialiasedLineEnable; }
    void SetForcedSampleCount(uint32_t forcedSampleCount) { m_RasterizerStates.ForcedSampleCount = forcedSampleCount; }
    void SetConservativeRaster(D3D12_CONSERVATIVE_RASTERIZATION_MODE conservativeRaster) { m_RasterizerStates.ConservativeRaster = conservativeRaster; }
    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) { m_PrimitiveTopology = topology; }
    void SetRenderTargetFormat(uint32_t idx, DXGI_FORMAT format);

private:
    GfxRootSignature*           m_RootSig = nullptr;
    GfxVertexFormat*            m_VertexFormat = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    GfxShader*                  m_VS = nullptr;
    GfxShader*                  m_PS = nullptr;
    GfxShader*                  m_CS = nullptr;
    CD3DX12_BLEND_DESC          m_BlendStates{ CD3DX12_DEFAULT{} };
    CD3DX12_DEPTH_STENCIL_DESC1 m_DepthStencilStates{ CD3DX12_DEFAULT{} };
    DXGI_FORMAT                 m_DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
    CD3DX12_RASTERIZER_DESC     m_RasterizerStates{ CD3DX12_DEFAULT{} };
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
};

#include "graphic/gfxpipelinestateobject.hpp"
