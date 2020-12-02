#pragma once

#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxvertexformat.h>
#include <graphic/gfx/gfxshadermanager.h>

class GfxManager;
class GfxCommandList;
class GfxTexture;
class GfxVertexBuffer;
class GfxIndexBuffer;
class GfxHazardTrackedResource;
struct GfxShader;

class GfxContext
{
public:
    void Initialize(D3D12_COMMAND_LIST_TYPE cmdListType, std::string_view name);

    void ResetToDefaultGraphicStates() { m_PSO = DefaultGraphicPSO(); }
    void ResetToDefaultComputeStates() { m_PSO = DefaultComputePSO(); }

    void ClearRenderTargetView(GfxTexture&, const bbeVector4& clearColor);
    void ClearDepth(GfxTexture&, float depth);
    void ClearDepthStencil(GfxTexture&, float depth, uint8_t stencil);

    // Interfaces for multiple render targets. Add more if needed
    void SetRenderTarget(GfxTexture&);
    void SetRenderTargets(GfxTexture&, GfxTexture&);
    void SetRenderTargets(GfxTexture&, GfxTexture&, GfxTexture&);

    void SetBlendStates(uint32_t renderTarget, const D3D12_RENDER_TARGET_BLEND_DESC& blendStates);
    void SetRasterizerStates(const CD3DX12_RASTERIZER_DESC& desc) { m_PSO.RasterizerState = desc; }
    void SetDepthStencilStates(const CD3DX12_DEPTH_STENCIL_DESC1& desc) { m_PSO.DepthStencilState = desc; }
    void SetViewport(const D3D12_VIEWPORT& vp);
    void SetRect(const D3D12_RECT& rect);
    void SetShader(const GfxShader&);
    void SetVertexFormat(const GfxVertexFormat& vertexFormat) { m_PSO.InputLayout = vertexFormat.Dev(); }
    void SetDepthStencil(GfxTexture& tex);
    void SetVertexBuffer(GfxVertexBuffer& vBuffer) { m_VertexBuffer = &vBuffer; }
    void SetIndexBuffer(GfxIndexBuffer& iBuffer) { m_IndexBuffer = &iBuffer; }
    void SetRootSignature(GfxRootSignature&);
    void StageSRV(GfxTexture&, uint32_t rootIndex, uint32_t offset);

    template <typename CBType>
    void StageCBV(const CBType& cb)
    {
        static_assert(CBType::ConstantBufferRegister != 0xDEADBEEF);
        StageCBVInternal((const void*)&cb, sizeof(CBType), CBType::ConstantBufferRegister, CBType::Name);
    }

    GfxCommandList& GetCommandList() { return *m_CommandList; }

    void TransitionResource(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate);
    void BeginResourceTransition(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);

    void DrawIndexedInstanced(uint32_t IndexCountPerInstance, uint32_t InstanceCount, uint32_t StartIndexLocation, uint32_t BaseVertexLocation, uint32_t StartInstanceLocation);

private:
    void PrepareGraphicsStates();
    void PreDraw();
    void PostDraw();
    void FlushResourceBarriers();
    void StageDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, uint32_t rootIndex, uint32_t offset);
    void CommitStagedResources();
    void CheckStagingResourceInputs(uint32_t rootIndex, uint32_t offset, D3D12_DESCRIPTOR_RANGE_TYPE);
    void StageCBVInternal(const void* data, uint32_t bufferSize, uint32_t cbRegister, const char* CBName);
    std::size_t GetPSOHash();

    template <uint32_t NbRTs>
    void SetRTVHelper(GfxTexture*(&RTVs)[NbRTs]);

    static CD3DX12_PIPELINE_STATE_STREAM2 DefaultGraphicPSO();
    static CD3DX12_PIPELINE_STATE_STREAM2 DefaultComputePSO();

    GfxRootSignature*            m_RootSig                      = nullptr;
    GfxVertexFormat*             m_VertexFormat                 = &GfxDefaultVertexFormats::Position2f_TexCoord2f_Color4ub;
    GfxCommandList*              m_CommandList                  = nullptr;
    GfxVertexBuffer*             m_VertexBuffer                 = nullptr;
    GfxIndexBuffer*              m_IndexBuffer                  = nullptr;
    InplaceArray<GfxTexture*, 3> m_RTVs;                        
    GfxTexture*                  m_DSV                          = nullptr;
    const GfxShader*             m_Shaders[GfxShaderType_Count] = {};

    CD3DX12_PIPELINE_STATE_STREAM2 m_PSO = DefaultGraphicPSO();

    InplaceArray<D3D12_RESOURCE_BARRIER, 16> m_ResourceBarriers;
    GfxRootSignature::RootSigParams m_StagedResources;

    std::bitset<GfxRootSignature::MaxRootParams> m_StaleResourcesBitMap;

    struct StagedCBV
    {
        const char* m_Name;
        InplaceArray<std::byte, 256> m_CBBytes;
    };
    StagedCBV m_StagedCBVs[gs_MaxRootSigParams]{};

    std::size_t m_LastBuffersHash = 0;
    std::size_t m_PSOHash = 0;

    friend class GfxManager;
};
