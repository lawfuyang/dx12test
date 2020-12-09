#pragma once

#include <graphic/gfx/gfxcommandlist.h>
#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxvertexformat.h>
#include <graphic/gfx/gfxshadermanager.h>

class GfxManager;
class GfxTexture;
class GfxVertexBuffer;
class GfxIndexBuffer;
class GfxHazardTrackedResource;
struct GfxShader;

class GfxContext
{
public:
    GfxCommandList& GetCommandList() { return *m_CommandList; }

    void Initialize(D3D12_COMMAND_LIST_TYPE cmdListType, std::string_view name);
    void ResetStates() { m_PSO = CD3DX12_PIPELINE_STATE_STREAM2{}; }

    void ClearRenderTargetView(GfxTexture&, const bbeVector4& clearColor);
    void ClearDepth(GfxTexture&, float depth);
    void ClearDepthStencil(GfxTexture&, float depth, uint8_t stencil);
    void ClearUAVF(GfxTexture&, const bbeVector4& clearValue);
    void ClearUAVU(GfxTexture&, const bbeVector4U& clearValue);

    // Interfaces for multiple render targets. Add more if needed
    void SetRenderTarget(GfxTexture&);
    void SetRenderTargets(GfxTexture&, GfxTexture&);
    void SetRenderTargets(GfxTexture&, GfxTexture&, GfxTexture&);

    void SetBlendStates(uint32_t renderTarget, const D3D12_RENDER_TARGET_BLEND_DESC& blendStates);

    template <typename T>
    void SetRasterizerStates(const T& desc) { m_PSO.RasterizerState = CD3DX12_RASTERIZER_DESC{ desc }; }

    template <typename T>
    void SetDepthStencilStates(const T& desc) { m_PSO.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1{ desc }; }

    void SetViewport(const D3D12_VIEWPORT& vp) { m_CommandList->Dev()->RSSetViewports(1, &vp); }
    void SetRect(const D3D12_RECT& rect) { m_CommandList->Dev()->RSSetScissorRects(1, &rect); }
    void SetShader(const GfxShader&);
    void SetVertexFormat(const GfxVertexFormat& vertexFormat) { m_PSO.InputLayout = vertexFormat.Dev(); }
    void SetDepthStencil(GfxTexture& tex);
    void SetVertexBuffer(GfxVertexBuffer& vBuffer) { m_VertexBuffer = &vBuffer; }
    void SetIndexBuffer(GfxIndexBuffer& iBuffer) { m_IndexBuffer = &iBuffer; }
    void SetRootSignature(GfxRootSignature&);
    void StageSRV(GfxTexture&, uint32_t rootIndex, uint32_t offset);
    void StageUAV(GfxTexture&, uint32_t rootIndex, uint32_t offset);

    template <typename CBType>
    void StageCBV(const CBType& cb)
    {
        static_assert(CBType::ConstantBufferRegister != 0xDEADBEEF);
        StageCBVInternal((const void*)&cb, sizeof(CBType), CBType::ConstantBufferRegister, CBType::Name);
    }

    void CreateRTV(const GfxTexture&);
    void CreateDSV(const GfxTexture&);
    void CreateSRV(const GfxTexture&);
    void CreateUAV(const GfxTexture&);

    void TransitionResource(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate);
    void BeginResourceTransition(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);

    void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation);
    void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, uint32_t startInstanceLocation);

    void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
private:
    void PrepareGraphicsStates();
    void PrepareComputeStates();
    void PostDraw();
    void FlushResourceBarriers();
    void StageDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, uint32_t rootIndex, uint32_t offset);
    void CommitStagedResources();
    void CheckStagingResourceInputs(uint32_t rootIndex, uint32_t offset, D3D12_DESCRIPTOR_RANGE_TYPE);
    void StageCBVInternal(const void* data, uint32_t bufferSize, uint32_t cbRegister, const char* CBName);
    void SetDescriptorHeapIfNeeded();
    std::size_t GetPSOHash();

    template <uint32_t NbRTs>
    void SetRTVHelper(GfxTexture*(&RTVs)[NbRTs]);

    GfxRootSignature*            m_RootSig                      = nullptr;
    GfxVertexFormat*             m_VertexFormat                 = &GfxDefaultVertexFormats::Position2f_TexCoord2f_Color4ub;
    GfxCommandList*              m_CommandList                  = nullptr;
    GfxVertexBuffer*             m_VertexBuffer                 = nullptr;
    GfxIndexBuffer*              m_IndexBuffer                  = nullptr;
    InplaceArray<GfxTexture*, 3> m_RTVs;                        
    GfxTexture*                  m_DSV                          = nullptr;
    const GfxShader*             m_Shaders[GfxShaderType_Count] = {};

    CD3DX12_PIPELINE_STATE_STREAM2 m_PSO;

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

    bool m_DescHeapsSet = false;

    friend class GfxManager;
};
