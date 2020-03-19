#pragma once

#include "graphic/gfxcommandlist.h"
#include "graphic/gfxcontext.h"
#include "graphic/gfxfence.h"
#include "graphic/gfxpipelinestateobject.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxdescriptorheap.h"
#include "graphic/gfxpipelinestateobject.h"

namespace D3D12MA
{
    class Allocator;
};

class GfxDevice
{
public:
    ID3D12Device6* Dev() const { return m_D3DDevice.Get(); }

    void Initialize();
    void ShutDown();
    void CheckStatus();
    void Flush(bool waitForPreviousFrame = false);
    void WaitForPreviousFrame();

    GfxContext& GenerateNewContext(D3D12_COMMAND_LIST_TYPE, const std::string& name);
    GfxCommandListsManager& GetCommandListsManager() { return m_CommandListsManager; }
    D3D12MA::Allocator& GetD3D12MemoryAllocator() { assert(m_D3D12MemoryAllocator); return *m_D3D12MemoryAllocator; }

    D3D_ROOT_SIGNATURE_VERSION GetHighSupportedRootSignature() const { return m_RootSigSupport.HighestVersion; }

private:
    void ConfigureDebugLayer();
    void CheckFeaturesSupports();
    void InitD3D12Allocator();

    GfxCommandListsManager   m_CommandListsManager;
    GfxFence                 m_GfxFence;

    std::vector<GfxContext> m_AllContexts;

    ComPtr<ID3D12Device6> m_D3DDevice;

    D3D12MA::Allocator* m_D3D12MemoryAllocator = nullptr;

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
