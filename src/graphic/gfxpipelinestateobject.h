#pragma once

#include "system/memorymappedfile.h"

class GfxRootSignature;
class GfxShader;

class GfxPipelineStateObject
{
public:
    void SetRootSignature(const GfxRootSignature* rootSig);
    void SetVertexInputLayout(const D3D12_INPUT_LAYOUT_DESC inputLayout) { m_InputLayout = inputLayout; }
    void SetVertexShader(GfxShader*);
    void SetPixelShader(GfxShader*);
    void SetComputeShader(GfxShader*);

private:
    template <typename ShaderStreamType>
    void SetShaderCommon(GfxShader*, ShaderStreamType&);

    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        m_RootSig;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          m_InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    m_PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS                    m_VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS                    m_PS;
    CD3DX12_PIPELINE_STATE_STREAM_CS                    m_CS;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            m_RasterizerState;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC            m_BlendState;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1        m_DepthStencilState;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK           m_StencilMask;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS m_RenderTargets;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           m_SampleDescriptors;
};

class GfxPSOManager
{
public:
    DeclareSingletonFunctions(GfxPSOManager);

    void Initialize();
    void ShutDown(bool deleteCacheFile);

    ID3D12PipelineState* GetPSO();

private:
    ComPtr<ID3D12PipelineLibrary> m_PipelineLibrary;

    MemoryMappedFile m_MemoryMappedCacheFile;
};
