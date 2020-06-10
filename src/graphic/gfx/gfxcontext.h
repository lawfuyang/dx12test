#pragma once

#include <system/profiler.h>

#include <graphic/gfx/gfxdescriptorheap.h>
#include <graphic/gfx/gfxpipelinestateobject.h>

class GfxManager;
class GfxDevice;
class GfxCommandList;
class GfxTexture;
class GfxRootSignature;
class GfxShaderManager;
class GfxVertexBuffer;
class GfxIndexBuffer;
class GfxDescriptorHeap;
class GfxConstantBuffer;
class GfxHazardTrackedResource;
class GfxBufferCommon;

class GfxContext
{
public:
    void Initialize(D3D12_COMMAND_LIST_TYPE cmdListType, const std::string& name);

    void ClearRenderTargetView(GfxTexture&, const bbeVector4& clearColor);
    void ClearDepth(GfxTexture&, float depth);
    void ClearDepthStencil(GfxTexture&, float depth, uint8_t stencil);
    void SetRenderTarget(uint32_t idx, GfxTexture&);
    void SetDepthStencil(GfxTexture& tex);
    void SetVertexBuffer(GfxVertexBuffer& vBuffer);
    void SetIndexBuffer(GfxIndexBuffer& iBuffer);
    void SetRootSignature(GfxRootSignature&);
    void StageSRV(GfxTexture&, uint32_t rootIndex, uint32_t offset);
    void StageCBV(GfxConstantBuffer&, uint32_t rootIndex, uint32_t offset);
    void SetRootSRV(GfxTexture&, uint32_t rootIndex);
    void SetRootCBV(GfxConstantBuffer&, uint32_t rootIndex);

    void DirtyPSO() { m_DirtyPSO = true; }

    static void CreateNullCBV(D3D12_CPU_DESCRIPTOR_HANDLE destDesc);
    static void CreateNullSRV(D3D12_CPU_DESCRIPTOR_HANDLE destDesc);
    static void CreateNullUAV(D3D12_CPU_DESCRIPTOR_HANDLE destDesc);
    static void CreateNullView(D3D12_DESCRIPTOR_RANGE_TYPE, D3D12_CPU_DESCRIPTOR_HANDLE destDesc);

    GfxCommandList&         GetCommandList() { return *m_CommandList; }
    GfxPipelineStateObject& GetPSO()         { return m_PSO; }

    void TransitionResource(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);

    void DrawIndexedInstanced(uint32_t IndexCountPerInstance, uint32_t InstanceCount, uint32_t StartIndexLocation, uint32_t BaseVertexLocation, uint32_t StartInstanceLocation);

private:
    struct StagedResourceDescriptor
    {
        std::vector<D3D12_DESCRIPTOR_RANGE_TYPE> m_SrcDescriptorTypes;
        std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_SrcDescriptors;
    };

    void CompileAndSetGraphicsPipelineState();
    void PreDraw();
    void PostDraw();
    void FlushResourceBarriers();
    void InsertUAVBarrier(GfxHazardTrackedResource& resource, bool flushImmediate = false);
    void StageDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, uint32_t rootIndex, uint32_t offset);
    void CommitStagedResources();
    void CheckStagingResourceInputs(uint32_t rootIndex, uint32_t offset, D3D12_DESCRIPTOR_RANGE_TYPE);
    void CheckRootResourceInputs(uint32_t rootIndex, D3D12_ROOT_PARAMETER_TYPE, const GfxDescriptorHeap&);

    GfxCommandList*   m_CommandList                                      = nullptr;
    GfxVertexBuffer*  m_VertexBuffer                                     = nullptr;
    GfxIndexBuffer*   m_IndexBuffer                                      = nullptr;
    GfxTexture*       m_RTVs[_countof(D3D12_RT_FORMAT_ARRAY::RTFormats)] = {};
    GfxTexture*       m_DSV                                              = nullptr;

    GfxPipelineStateObject m_PSO;

    InplaceArray<D3D12_RESOURCE_BARRIER, 16> m_ResourceBarriers;
    InplaceArray<StagedResourceDescriptor, GfxRootSignature::MaxRootParams> m_StagedResources;

    std::bitset<GfxRootSignature::MaxRootParams> m_StaleResourcesBitMap;

    bool m_DirtyPSO        = true;
    bool m_DirtyBuffers    = true;

    friend class GfxManager;
};
