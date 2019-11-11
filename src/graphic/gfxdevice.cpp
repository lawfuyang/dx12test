#include "graphic/gfxdevice.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxadapter.h"
#include "graphic/gfxenums.h"

GfxDevice::GfxDevice()
{

}

GfxDevice::~GfxDevice()
{

}

void GfxDevice::InitializeForMainDevice()
{
    bbeProfileFunction();

    m_DeviceType = GfxDeviceType::MainDevice;

    if constexpr (System::EnableGfxDebugLayer)
    {
        EnableDebugLayer();
    }

    constexpr D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    for (D3D_FEATURE_LEVEL level : featureLevels)
    {
        bbeProfile("D3D12CreateDevice");

        if (SUCCEEDED(D3D12CreateDevice(GfxAdapter::GetInstance().GetAllAdapters()[0].Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_D3DDevice))))
        {
            bbeInfo("Created ID3D12Device. D3D_FEATURE_LEVEL: %s", GetD3DFeatureLevelName(level));
            break;
        }
    }
    bbeAssert(m_D3DDevice.Get() != nullptr, "ID3D12Device not successfully created?");

    if constexpr (System::EnableGfxDebugLayer)
    {
        ConfigureDebugLayer();
    }

    m_GfxCommandQueue.Initialize(this, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_SwapChain.Initialize(this, System::APP_WINDOW_WIDTH, System::APP_WINDOW_HEIGHT, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void GfxDevice::EnableDebugLayer()
{
    bbeProfileFunction();

    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
        {
            debugController1->SetEnableGPUBasedValidation(System::GfxDebugLayerEnableGPUValidation);
            debugController1->SetEnableSynchronizedCommandQueueValidation(System::GfxDebugLayerSynchronizedCommandQueueValidation);
        }

        ComPtr<ID3D12Debug2> debugController2;
        if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController2))))
        {
            debugController2->SetGPUBasedValidationFlags(System::GfxDebugLayerDisableStateTracking ? D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING : D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
        }
    }
}

void GfxDevice::ConfigureDebugLayer()
{
    bbeProfileFunction();

    ComPtr<ID3D12DebugDevice1> debugDevice1;
    if (SUCCEEDED(m_D3DDevice.Get()->QueryInterface(IID_PPV_ARGS(&debugDevice1))))
    {
        if (System::GfxDebugLayerEnableConservativeResorceStateTracking)
        {
            const D3D12_DEBUG_FEATURE debugFeatures
            {
                D3D12_DEBUG_FEATURE_ALLOW_BEHAVIOR_CHANGING_DEBUG_AIDS | D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING
            };
            debugDevice1->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_FEATURE_FLAGS, &debugFeatures, sizeof(debugFeatures));
        }

        if (System::GfxDebugLayerEnableGPUValidation)
        {
            const D3D12_DEBUG_DEVICE_GPU_BASED_VALIDATION_SETTINGS gpuValidationSettings
            {
                256,
                D3D12_GPU_BASED_VALIDATION_SHADER_PATCH_MODE_GUARDED_VALIDATION, // erroneous instructions are skipped in execution
                D3D12_GPU_BASED_VALIDATION_PIPELINE_STATE_CREATE_FLAG_NONE       // on the fly creation of patch PSO
            };
            debugDevice1->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_GPU_BASED_VALIDATION_SETTINGS, &gpuValidationSettings, sizeof(gpuValidationSettings));
        }
    }

    ComPtr<ID3D12InfoQueue> debugInfoQueue;
    DX12_CALL(m_D3DDevice.Get()->QueryInterface(__uuidof(ID3D12InfoQueue), (LPVOID*)&debugInfoQueue));

    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, System::GfxDebugLayerBreakOnErrors);
    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, System::GfxDebugLayerBreakOnWarnings);
}
