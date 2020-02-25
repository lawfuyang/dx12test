#pragma once

#include "system/profiler.h"

#include "graphic/gfxpipelinestateobject.h"

class GfxManager;
class GfxDevice;
class GfxCommandList;
class GfxTexture;
class GfxRootSignature;
class GfxShaderManager;
class GfxVertexBuffer;
class GfxIndexBuffer;
class GfxDescriptorHeap;

class GfxContext
{
public:
    void ClearRenderTargetView(GfxTexture&, XMFLOAT4 clearColor) const;
    void SetRenderTarget(uint32_t idx, GfxTexture&);
    void SetVertexBuffer(GfxVertexBuffer& vBuffer) { m_VertexBuffer = &vBuffer; }
    void SetIndexBuffer(GfxIndexBuffer& iBuffer) { m_IndexBuffer = &iBuffer; }
    void BindConstantBuffer(GfxConstantBuffer& cBuffer);

    GfxDevice&              GetDevice()             { return *m_Device; }
    GfxCommandList&         GetCommandList()        { return *m_CommandList; }
    GfxPipelineStateObject& GetPSO()                { return m_PSO; }
    GPUProfilerContext&     GetGPUProfilerContext() { return m_GPUProfilerContext; }

    void DrawInstanced(uint32_t VertexCountPerInstance, uint32_t InstanceCount, uint32_t StartVertexLocation, uint32_t StartInstanceLocation);
    void DrawIndexedInstanced(uint32_t InstanceCount, uint32_t StartIndexLocation, uint32_t BaseVertexLocation, uint32_t StartInstanceLocation);

private:
    void SetRootSigDescTable(uint32_t rootParamIdx, const GfxDescriptorHeap& heap);
    void CompileAndSetGraphicsPipelineState();
    void CompileAndSetComputePipelineState();

    uint32_t m_ID = 0xDEADBEEF;

    CD3DX12_VIEWPORT  m_Viewport{ 0.0f, 0.0f, System::APP_WINDOW_WIDTH, System::APP_WINDOW_HEIGHT };
    CD3DX12_RECT      m_ScissorRect{ 0, 0, static_cast<LONG>(System::APP_WINDOW_WIDTH), static_cast<LONG>(System::APP_WINDOW_HEIGHT) };

    GfxDevice*         m_Device                                           = nullptr;
    GfxCommandList*    m_CommandList                                      = nullptr;
    GfxVertexBuffer*   m_VertexBuffer                                     = nullptr;
    GfxIndexBuffer*    m_IndexBuffer                                      = nullptr;
    GfxTexture*        m_RTVs[_countof(D3D12_RT_FORMAT_ARRAY::RTFormats)] = {};
    GfxDescriptorHeap* m_CBVToBind                                        = nullptr;

    GfxPipelineStateObject m_PSO;

    GPUProfilerContext m_GPUProfilerContext;

    friend class GfxDevice;
};
