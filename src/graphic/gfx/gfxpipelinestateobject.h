#pragma once

#include <system/memorymappedfile.h>

#include <graphic/dx12utils.h>
#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxvertexformat.h>
#include <graphic/gfx/gfxshadermanager.h>
#include <graphic/gfx/gfxcommonstates.h>

class GfxRootSignature;
class GfxShader;

struct GfxPipelineStateObject
{
public:
    void SetRootSignature(GfxRootSignature& rootSig) { m_RootSig = &rootSig; }
    void SetVertexFormat(GfxVertexFormat& inputLayout) { m_VertexFormat = &inputLayout; }
    void SetVertexShader(const GfxShader& shader) { m_VS = &shader; }
    void SetPixelShader(const GfxShader& shader) { m_PS = &shader; }
    void SetComputeShader(const GfxShader& shader) { m_CS = &shader; }
    void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) { m_PrimitiveTopology = topology; }
    void SetRenderTargetFormat(uint32_t idx, DXGI_FORMAT format);
    void SetDepthStencilFormat(DXGI_FORMAT format) { m_DepthStencilFormat = format; };
    CD3DX12_BLEND_DESC& GetBlendStates() { return m_BlendStates; }
    CD3DX12_RASTERIZER_DESC& GetRasterizerStates() { return m_RasterizerStates; }
    CD3DX12_DEPTH_STENCIL_DESC1& GetDepthStencilStates() { return m_DepthStencilStates; }

    std::size_t GetHash() const { return m_Hash; }

private:
    GfxRootSignature*           m_RootSig = nullptr;
    GfxVertexFormat*            m_VertexFormat = &GfxDefaultVertexFormats::Position2f_TexCoord2f_Color4ub;
    D3D12_PRIMITIVE_TOPOLOGY    m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    const GfxShader*            m_VS = nullptr;
    const GfxShader*            m_PS = nullptr;
    const GfxShader*            m_CS = nullptr;
    CD3DX12_BLEND_DESC          m_BlendStates{ GfxCommonStates::Opaque };
    CD3DX12_DEPTH_STENCIL_DESC1 m_DepthStencilStates{ GfxCommonStates::DepthDefault };
    DXGI_FORMAT                 m_DepthStencilFormat = DXGI_FORMAT_UNKNOWN;
    CD3DX12_RASTERIZER_DESC     m_RasterizerStates{ GfxCommonStates::CullCounterClockwise };
    D3D12_RT_FORMAT_ARRAY       m_RenderTargets = {};
    DXGI_SAMPLE_DESC            m_SampleDescriptors = DefaultSampleDesc{};

    // TODO
    // CD3DX12_PIPELINE_STATE_STREAM2 m_States{};

    std::size_t m_Hash = 0;

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
    template <typename PSODesc>
    ID3D12PipelineState* LoadOrSavePSOFromLibrary(const GfxPipelineStateObject& pso, const PSODesc&);

    bool LoadPSOInternal(LPCWSTR pName, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pDesc, ID3D12PipelineState*& pso) { return m_PipelineLibrary->LoadGraphicsPipeline(pName, &pDesc, IID_PPV_ARGS(&pso)) != E_INVALIDARG; }
    bool LoadPSOInternal(LPCWSTR pName, const D3D12_COMPUTE_PIPELINE_STATE_DESC& pDesc, ID3D12PipelineState*& pso) { return m_PipelineLibrary->LoadComputePipeline(pName, &pDesc, IID_PPV_ARGS(&pso)) != E_INVALIDARG; }

    std::mutex m_PipelineLibraryLock;
    ComPtr<ID3D12PipelineLibrary1> m_PipelineLibrary;

    MemoryMappedFile m_MemoryMappedCacheFile;

    uint32_t m_NewPSOs = 0;
};
#define g_GfxPSOManager GfxPSOManager::GetInstance()

#include "graphic/gfx/gfxpipelinestateobject.hpp"
