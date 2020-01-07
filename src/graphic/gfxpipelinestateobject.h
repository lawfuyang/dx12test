#pragma once

#include "system/memorymappedfile.h"

class GfxRootSignature;
class GfxShader;

class GfxPipelineStateObject
{
public:
    void SetRootSignature(const GfxRootSignature*);
    void SetVertexInputLayout(const D3D12_INPUT_LAYOUT_DESC inputLayout) { m_InputLayout = inputLayout; }
    void SetVertexShader(GfxShader&);
    void SetPixelShader(GfxShader&);
    void SetComputeShader(GfxShader&);
    void SetDepthEnable(bool b) { static_cast<CD3DX12_DEPTH_STENCIL_DESC1&>(m_DepthStencilState).DepthEnable = b; }
    void SetStencilEnable(bool b) { static_cast<CD3DX12_DEPTH_STENCIL_DESC1&>(m_DepthStencilState).StencilEnable = b; }
    void SetFillMode(D3D12_FILL_MODE fillMode) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).FillMode = fillMode; }
    void SetCullMode(D3D12_CULL_MODE cullMode) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).CullMode = cullMode; }
    void SetFrontCounterClockwise(bool b) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).FrontCounterClockwise = b; }
    void SetDepthBias(uint32_t depthBias) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).DepthBias = depthBias; }
    void SetDepthBiasClamp(float depthBiasClamp) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).DepthBiasClamp = depthBiasClamp; }
    void SetSlopeScaledDepthBias(float slopeScaledDepthBias) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).SlopeScaledDepthBias = slopeScaledDepthBias; }
    void SetDepthClipEnable(bool depthClipEnable) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).DepthClipEnable = depthClipEnable; }
    void SetMultisampleEnable(bool multisampleEnable) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).MultisampleEnable = multisampleEnable; }
    void SetAntialiasedLineEnable(bool antialiasedLineEnable) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).AntialiasedLineEnable = antialiasedLineEnable; }
    void SetForcedSampleCount(uint32_t forcedSampleCount) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).ForcedSampleCount = forcedSampleCount; }
    void SetConservativeRaster(D3D12_CONSERVATIVE_RASTERIZATION_MODE conservativeRaster) { static_cast<CD3DX12_RASTERIZER_DESC&>(m_RasterizerState).ConservativeRaster = conservativeRaster; }
    void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType) { m_PrimitiveTopologyType = topologyType; }
    void SetRenderTargetFormat(uint32_t idx, DXGI_FORMAT format);

private:
    template <typename ShaderStreamType>
    void SetShaderCommon(GfxShader&, ShaderStreamType&);

    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        m_RootSig;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          m_InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    m_PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS                    m_VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS                    m_PS;
    CD3DX12_PIPELINE_STATE_STREAM_CS                    m_CS;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC            m_BlendState;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1        m_DepthStencilState;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  m_DepthStencilFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            m_RasterizerState;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS m_RenderTargets;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           m_SampleDescriptors;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK           m_StencilMask;
};

class GfxPSOManager
{
public:
    DeclareSingletonFunctions(GfxPSOManager);

    void Initialize();
    void ShutDown(bool deleteCacheFile);

    ID3D12PipelineState* GetPSO(const GfxPipelineStateObject&);

private:
    ComPtr<ID3D12PipelineLibrary> m_PipelineLibrary;

    MemoryMappedFile m_MemoryMappedCacheFile;
};
