#pragma once

#include "graphic/gfxcommandlist.h"
#include "graphic/gfxfence.h"
#include "graphic/gfxpipelinestate.h"
#include "graphic/gfxswapchain.h"

class GfxDevice
{
public:
    ID3D12Device* Dev() const { return m_D3DDevice.Get(); }

    void Initialize();
    void CheckStatus();
    void EndFrame();
    void WaitForPreviousFrame();

    GfxContext GenerateNewContext(D3D12_COMMAND_LIST_TYPE);
    GfxCommandListsManager& GetCommandListsManager() { return m_CommandListsManager; }

private:
    static void EnableDebugLayer();
    void ConfigureDebugLayer();

    GfxCommandListsManager m_CommandListsManager;
    GfxFence m_GfxFence;

    ComPtr<ID3D12Device6> m_D3DDevice;
};
