#pragma once

namespace D3D12MA
{
    class Allocator;
};

class GfxDevice
{
public:
    D3D12Device* Dev() const { return m_D3DDevice.Get(); }

    void Initialize();
    void CheckStatus();

    D3D_ROOT_SIGNATURE_VERSION GetHighSupportedRootSignature() const { return m_RootSigSupport.HighestVersion; }

private:
    void ConfigureDebugLayerAfterDeviceCreation();
    void CheckFeaturesSupports();

    ComPtr<D3D12Device> m_D3DDevice;

    bool m_TearingSupported = false;
    D3D12_FEATURE_DATA_D3D12_OPTIONS               m_D3D12Options                  = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS1              m_D3D12Options1                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS2              m_D3D12Options2                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS3              m_D3D12Options3                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS4              m_D3D12Options4                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS5              m_D3D12Options5                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS6              m_D3D12Options6                 = {};
    D3D12_FEATURE_DATA_D3D12_OPTIONS7              m_D3D12Options7                 = {};
    D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT m_D3D12GPUVirtualAddressSupport = {};
    D3D12_FEATURE_DATA_SHADER_MODEL                m_D3DHighestShaderModel         = { D3D_SHADER_MODEL_5_1 };
    D3D12_FEATURE_DATA_ROOT_SIGNATURE              m_RootSigSupport                = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
};

class GfxMemoryAllocator
{
    DeclareSingletonFunctions(GfxMemoryAllocator);

public:
    void Initialize();

    D3D12MA::Allocator& Dev() { assert(m_D3D12MemoryAllocator); return *m_D3D12MemoryAllocator; }

    D3D12MA::Allocation* Allocate(D3D12_HEAP_TYPE, const CD3DX12_RESOURCE_DESC1&, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE*);
    void Release(D3D12MA::Allocation*);

private:
    D3D12MA::Allocator* m_D3D12MemoryAllocator = nullptr;
};
#define g_GfxMemoryAllocator GfxMemoryAllocator::GetInstance()

void DeviceRemovedHandler();
