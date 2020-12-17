#pragma once

#include <graphic/gfx/gfxcommandlist.h>
#include <graphic/gfx/gfxdescriptorheap.h>
#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxvertexformat.h>
#include <graphic/gfx/gfxshadermanager.h>

class GfxManager;
class GfxTexture;
class GfxVertexBuffer;
class GfxIndexBuffer;
class GfxResourceBase;
class GfxSwapChain;
class View;
struct GfxShader;

class GfxContext
{
public:
    GfxCommandList& GetCommandList() { return *m_CommandList; }

    void Initialize(D3D12_COMMAND_LIST_TYPE cmdListType, std::string_view name);
    void ResetStates() { m_PSO = CD3DX12_PIPELINE_STATE_STREAM2{}; }

    void BeginTrackResourceState(GfxResourceBase& resource, D3D12_RESOURCE_STATES assumedInitialState);
    D3D12_RESOURCE_STATES GetCurrentResourceState(GfxResourceBase& resource) const;

    void ClearRenderTargetView(GfxTexture&, const bbeVector4& clearColor);
    void ClearDepth(GfxTexture& tex, float depth) { ClearDSVInternal(tex, depth, 0, D3D12_CLEAR_FLAG_DEPTH); }
    void ClearDepthStencil(GfxTexture& tex, float depth, uint8_t stencil) { ClearDSVInternal(tex, depth, stencil, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL); }
    void ClearUAVF(GfxTexture&, const bbeVector4& clearValue);
    void ClearUAVU(GfxTexture&, const bbeVector4U& clearValue);

    // Interfaces for multiple render targets. Add more if needed
    void SetRenderTarget(GfxTexture&);
    void SetRenderTargets(GfxTexture&, GfxTexture&);
    void SetRenderTargets(GfxTexture&, GfxTexture&, GfxTexture&);
    
    void SetTopology(D3D12_PRIMITIVE_TOPOLOGY topology);
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
    void SetVertexBuffer(GfxVertexBuffer& vBuffer);
    void SetIndexBuffer(GfxIndexBuffer& iBuffer);
    void SetRootSignature(std::size_t rootSigHash);
    void StageSRV(GfxTexture&, uint32_t rootIndex, uint32_t offset);
    void StageUAV(GfxTexture&, uint32_t rootIndex, uint32_t offset);

    template <typename CBType>
    void StageCBV(const CBType& cb)
    {
        static_assert(CBType::ConstantBufferRegister != 0xDEADBEEF);
        StageCBVInternal((const void*)&cb, sizeof(CBType), CBType::ConstantBufferRegister, CBType::Name);
    }

    void UAVBarrier(GfxResourceBase& resource);
    void TransitionResource(GfxResourceBase& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);

    void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0);
    void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation = 0, uint32_t baseVertexLocation = 0, uint32_t startInstanceLocation = 0);

    void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

private:
    using RootDescTableSetter = void (STDMETHODCALLTYPE D3D12GraphicsCommandList::*)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE);

    void PrepareGraphicsStates();
    void PrepareComputeStates();
    void PostDraw();
    void FlushResourceBarriers();
    void StageDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, uint32_t rootIndex, uint32_t offset);
    void CommitStagedResources(RootDescTableSetter rootDescSetterFunc);
    void CheckStagingResourceInputs(uint32_t rootIndex, uint32_t offset, D3D12_DESCRIPTOR_RANGE_TYPE);
    void StageCBVInternal(const void* data, uint32_t bufferSize, uint32_t cbRegister, const char* CBName);
    void SetShaderVisibleDescriptorHeap();
    void ClearDSVInternal(GfxTexture& tex, float depth, uint8_t stencil, D3D12_CLEAR_FLAGS flags);
    CD3DX12_CPU_DESCRIPTOR_HANDLE AllocateStaticDescHeap(D3D12_DESCRIPTOR_HEAP_TYPE, std::string_view debugName);
    std::size_t GetPSOHash(bool forGraphicPSO);

    void LazyTransitionResource(GfxResourceBase& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);

    template <uint32_t NbRTs>
    void SetRTVHelper(GfxTexture*(&RTVs)[NbRTs]);

    const GfxRootSignature*  m_RootSig                      = nullptr;
    GfxVertexFormat*         m_VertexFormat                 = &GfxDefaultVertexFormats::Position2f_TexCoord2f_Color4ub;
    GfxCommandList*          m_CommandList                  = nullptr;
    GfxVertexBuffer*         m_VertexBuffer                 = nullptr;
    GfxIndexBuffer*          m_IndexBuffer                  = nullptr;
    GfxTexture*              m_DSV                          = nullptr;
    const GfxShader*         m_Shaders[GfxShaderType_Count] = {};
    D3D12_PRIMITIVE_TOPOLOGY m_Topology                     = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    struct RTVDesc
    {
        GfxTexture* m_Tex = nullptr;
        CD3DX12_CPU_DESCRIPTOR_HANDLE m_CPUDescHeap{};
    };
    InplaceArray<RTVDesc, 3> m_RTVs;

    CD3DX12_PIPELINE_STATE_STREAM2 m_PSO;

    InplaceFlatMap<D3D12Resource*, D3D12_RESOURCE_STATES, 8> m_TrackedResourceStates;

    InplaceArray<D3D12_RESOURCE_BARRIER, 16> m_BegunResourceBarriers;
    InplaceArray<D3D12_RESOURCE_BARRIER, 16> m_ResourceBarriers;
    GfxRootSignature::RootSigParams m_StagedResources;

    std::bitset<GfxRootSignature::MaxRootParams> m_StaleResourcesBitMap;

    CircularBuffer<GfxDescriptorHeap> m_StaticHeaps;

    struct StagedCBV
    {
        const char* m_Name;
        InplaceArray<std::byte, 256> m_CBBytes;
    };
    StagedCBV m_StagedCBVs[gs_MaxRootSigParams]{};

    std::size_t m_LastBuffersHash = 0;
    std::size_t m_PSOHash = 0;

    bool m_ShaderVisibleDescHeapsSet = false;

    friend class GfxManager;
};
