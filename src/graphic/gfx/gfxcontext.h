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

    template <typename CBType>
    void StageCBV(const CBType& cb)
    {
        static_assert(CBType::ConstantBufferRegister != 0xDEADBEEF);
        StageCBVInternal((const void*)&cb, sizeof(CBType), CBType::ConstantBufferRegister, CBType::Name);
    }

    GfxCommandList&         GetCommandList() { return *m_CommandList; }
    GfxPipelineStateObject& GetPSO()         { return m_PSO; }

    void TransitionResource(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);

    void DrawIndexedInstanced(uint32_t IndexCountPerInstance, uint32_t InstanceCount, uint32_t StartIndexLocation, uint32_t BaseVertexLocation, uint32_t StartInstanceLocation);

private:
    struct StagedCBV
    {
        const char* m_Name;
        InplaceArray<std::byte, 256> m_CBBytes;
    };

    void CompileAndSetGraphicsPipelineState();
    void PreDraw();
    void PostDraw();
    void FlushResourceBarriers();
    void InsertUAVBarrier(GfxHazardTrackedResource& resource, bool flushImmediate = false);
    void StageDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor, uint32_t rootIndex, uint32_t offset);
    void CommitStagedResources();
    void CheckStagingResourceInputs(uint32_t rootIndex, uint32_t offset, D3D12_DESCRIPTOR_RANGE_TYPE);
    void StageCBVInternal(const void* data, uint32_t bufferSize, uint32_t cbRegister, const char* CBName);

    GfxCommandList*   m_CommandList                                      = nullptr;
    GfxVertexBuffer*  m_VertexBuffer                                     = nullptr;
    GfxIndexBuffer*   m_IndexBuffer                                      = nullptr;
    GfxTexture*       m_RTVs[_countof(D3D12_RT_FORMAT_ARRAY::RTFormats)] = {};
    GfxTexture*       m_DSV                                              = nullptr;

    GfxPipelineStateObject m_PSO;

    InplaceArray<D3D12_RESOURCE_BARRIER, 16> m_ResourceBarriers;
    GfxRootSignature::RootSigParams m_StagedResources;

    std::bitset<GfxRootSignature::MaxRootParams> m_StaleResourcesBitMap;
    std::unordered_map<uint32_t, StagedCBV> m_StagedCBVs;

    std::size_t m_LastUsedPSOHash = 0;

    bool m_DirtyBuffers = true;

    friend class GfxManager;
};
