#pragma once

#include "graphic/gfxcommandlist.h"
#include "graphic/gfxfence.h"
#include "graphic/gfxpipelinestateobject.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxdescriptorheap.h"
#include "graphic/gfxpipelinestateobject.h"

class GfxDevice
{
public:
    ID3D12Device* Dev() const { return m_D3DDevice.Get(); }

    void Initialize();
    void CheckStatus();
    void EndFrame();
    void WaitForPreviousFrame();

    GfxContext& GenerateNewContext(D3D12_COMMAND_LIST_TYPE);
    GfxCommandListsManager& GetCommandListsManager() { return m_CommandListsManager; }
    GfxDescriptorHeapManager& GetDescriptorHeapManager() { return m_DescriptorHeapManager; }

    D3D_ROOT_SIGNATURE_VERSION GetHighSupportedRootSignature() const { return m_RootSigSupport.HighestVersion; }

private:
    static void EnableDebugLayer();
    void ConfigureDebugLayer();
    void CheckFeaturesSupports();

    GfxCommandListsManager   m_CommandListsManager;
    GfxDescriptorHeapManager m_DescriptorHeapManager;
    GfxFence                 m_GfxFence;

    std::vector<GfxContext> m_AllContexts;

    ComPtr<ID3D12Device6> m_D3DDevice;

    bool m_TearingSupported = false;
    D3D12_FEATURE_DATA_D3D12_OPTIONS               m_D3D12Options                  = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS1              m_D3D12Options1                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS2              m_D3D12Options2                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS3              m_D3D12Options3                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS4              m_D3D12Options4                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS5              m_D3D12Options5                 = {};
    D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT m_D3D12GPUVirtualAddressSupport = {};
    D3D12_FEATURE_DATA_SHADER_MODEL                m_D3DHighestShaderModel         = { D3D_SHADER_MODEL_5_1 };
    D3D12_FEATURE_DATA_ROOT_SIGNATURE              m_RootSigSupport                = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
};
