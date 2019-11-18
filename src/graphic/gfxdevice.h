#pragma once

#include "graphic/gfxcommandqueue.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxcommandlist.h"
#include "graphic/gfxfence.h"

enum class GfxDeviceType
{
    Main,
    Deferred,
    LoadingThread,
    Compute,
    Copy,
    Creation
};

class GfxDevice
{
public:
    ID3D12Device* Dev() const { return m_D3DDevice.Get(); }

    ID3D12CommandAllocator* GetCommandAllocator() const { return m_CommandAllocator.Get(); }
    ID3D12PipelineState* GetPipelineState() const { return m_PipelineState.Get(); }
    GfxCommandQueue& GetCommandQueue() { return m_GfxCommandQueue; }
    GfxCommandList& GetCommandList() { return m_GfxCommandList; }

    void InitializeForMainDevice();
    void CheckStatus();
    void BeginFrame();
    void EndFrame();
    void WaitForPreviousFrame();

    // TODO: Convert to use math lib's vec4
    void ClearRenderTargetView(GfxRenderTargetView&, const float(&)[4]);

private:
    static void EnableDebugLayer();
    void ConfigureDebugLayer();

    GfxDeviceType m_DeviceType;

    GfxCommandQueue m_GfxCommandQueue;
    GfxCommandList m_GfxCommandList;
    GfxFence m_GfxFence;

    ComPtr<ID3D12Device6> m_D3DDevice;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<ID3D12PipelineState> m_PipelineState;
};
