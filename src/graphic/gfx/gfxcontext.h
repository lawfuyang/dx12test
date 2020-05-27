#pragma once

#include "system/profiler.h"

#include "graphic/gfx/gfxpipelinestateobject.h"

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

class GfxContext
{
public:
    void ClearRenderTargetView(GfxTexture&, const bbeVector4& clearColor);
    void ClearDepthStencilView(GfxTexture&, float depth, uint8_t stencil);
    void SetRenderTarget(uint32_t idx, GfxTexture&);
    void SetDepthStencil(GfxTexture& tex);
    void SetVertexBuffer(GfxVertexBuffer& vBuffer);
    void SetIndexBuffer(GfxIndexBuffer& iBuffer);
    void BindConstantBuffer(GfxConstantBuffer& cBuffer);
    void BindSRV(uint32_t textureRegister, GfxTexture&);

    void DirtyPSO() { m_DirtyPSO = true; }
    void DirtyRasterizer() { m_DirtyRasterizer = true; }
    void DirtyDescTables() { m_DirtyDescTables = true; }

    GfxCommandList&         GetCommandList() { return *m_CommandList; }
    GfxPipelineStateObject& GetPSO()         { return m_PSO; }
    CD3DX12_RECT&           GetScissorRect() { return m_ScissorRect; }
    CD3DX12_VIEWPORT&       GetViewport()    { return m_Viewport; }

    void TransitionResource(GfxHazardTrackedResource& resource, D3D12_RESOURCE_STATES newState, bool flushImmediate = false);

    void DrawInstanced(uint32_t VertexCountPerInstance, uint32_t InstanceCount, uint32_t StartVertexLocation, uint32_t StartInstanceLocation);
    void DrawIndexedInstanced(uint32_t IndexCountPerInstance, uint32_t InstanceCount, uint32_t StartIndexLocation, uint32_t BaseVertexLocation, uint32_t StartInstanceLocation);

private:
    void SetRootSigDescTable(uint32_t rootParamIdx, const GfxDescriptorHeap& heap);
    void CompileAndSetGraphicsPipelineState();
    void CompileAndSetComputePipelineState();
    void PostDraw();
    void FlushResourceBarriers();
    void InsertUAVBarrier(GfxHazardTrackedResource& resource, bool flushImmediate = false);

    CD3DX12_VIEWPORT  m_Viewport{ 0.0f, 0.0f, (float)g_CommandLineOptions.m_WindowWidth, (float)g_CommandLineOptions.m_WindowHeight };
    CD3DX12_RECT      m_ScissorRect{ 0, 0, g_CommandLineOptions.m_WindowWidth, g_CommandLineOptions.m_WindowHeight };

    GfxCommandList*          m_CommandList                                      = nullptr;
    GfxVertexBuffer*         m_VertexBuffer                                     = nullptr;
    GfxIndexBuffer*          m_IndexBuffer                                      = nullptr;
    GfxTexture*              m_RTVs[_countof(D3D12_RT_FORMAT_ARRAY::RTFormats)] = {};
    GfxTexture*              m_DSV                                              = nullptr;
    GfxDescriptorHeap*       m_CBVToBind                                        = nullptr;
    std::vector<GfxTexture*> m_SRVsToBind;

    GfxPipelineStateObject m_PSO;

    std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers;

    bool m_DirtyPSO        = true;
    bool m_DirtyRasterizer = true;
    bool m_DirtyBuffers    = true;
    bool m_DirtyDescTables = true;

    friend class GfxManager;
};