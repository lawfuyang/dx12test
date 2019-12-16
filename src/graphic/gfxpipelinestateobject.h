#pragma once

#include "system/memorymappedfile.h"

struct GfxGraphicsPSOStream
{
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          m_InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    m_PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        m_RootSig;
    CD3DX12_PIPELINE_STATE_STREAM_VS                    m_VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS                    m_PS;
    CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            m_RasterizerState;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC            m_BlendState;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1        m_DepthStencilState;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK           m_StencilMask;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS m_RenderTargets;
    CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           m_SampleDescriptors;
};

struct GfxComputePSOStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE m_RootSig;
    CD3DX12_PIPELINE_STATE_STREAM_CS             m_CS;
};

class GfxVertexInputLayout
{
public:
    const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetDescs() const { return m_InputElementDescs; }

    void AddElement(const D3D12_INPUT_ELEMENT_DESC& elem) { m_InputElementDescs.push_back(elem); }

private:
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputElementDescs;
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
