#pragma once

#include "extern/boost/pool/object_pool.hpp"

#include "graphic/gfxcommandqueue.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxcommandlist.h"
#include "graphic/gfxfence.h"
#include "graphic/gfxpipelinestate.h"

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

    GfxCommandQueue& GetCommandQueue() { return m_GfxCommandQueue; }

    void InitializeForMainDevice();
    void CheckStatus();
    void ExecuteAllActiveCommandLists();
    void WaitForPreviousFrame();

    GfxCommandList* NewCommandList(const std::string& cmdListName = "");
    GfxCommandList* GetCurrentCommandList() { return m_CurrentCommandList; }

    // TODO: Convert to use math lib's vec4
    void ClearRenderTargetView(GfxRenderTargetView&, const float(&)[4]);

private:
    static void EnableDebugLayer();
    void ConfigureDebugLayer();

    GfxDeviceType m_DeviceType;

    boost::object_pool<GfxCommandList> m_CommandListsPool;

    GfxCommandList* m_CurrentCommandList = nullptr;
    std::vector<GfxCommandList*> m_FreeCommandLists;
    std::vector<GfxCommandList*> m_ActiveCommandLists;
    SpinLock m_FreeCommandListsLock;
    SpinLock m_ActiveCommandListsLock;

    GfxCommandQueue m_GfxCommandQueue;
    GfxFence m_GfxFence;
    GfxPipelineState m_PipelineState;

    ComPtr<ID3D12Device6> m_D3DDevice;
};
