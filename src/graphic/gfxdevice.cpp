#include "graphic/gfxdevice.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxadapter.h"
#include "graphic/gfxcontext.h"

void GfxDevice::Initialize()
{
    bbeProfileFunction();

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

        if (SUCCEEDED(D3D12CreateDevice(GfxAdapter::GetInstance().GetAllAdapters()[0].Get(), level, IID_PPV_ARGS(&m_D3DDevice))))
        {
            bbeInfo("Created ID3D12Device. D3D_FEATURE_LEVEL: %s", GetD3DFeatureLevelName(level));
            break;
        }
    }
    bbeAssert(m_D3DDevice.Get() != nullptr, "");

    if constexpr (System::EnableGfxDebugLayer)
    {
        ConfigureDebugLayer();
    }

    m_CommandListsManager.Initialize();

    m_GfxFence.Initialize(D3D12_FENCE_FLAG_NONE);
}

void GfxDevice::CheckStatus()
{
    const ::HRESULT result = m_D3DDevice->GetDeviceRemovedReason();

    bbeAssert(result != DXGI_ERROR_DEVICE_HUNG, "Graphics Device Hung");
    bbeAssert(result != DXGI_ERROR_DEVICE_REMOVED, "Graphics Device Removed");
    bbeAssert(result != DXGI_ERROR_DEVICE_RESET, "Graphics Device Reset");
    bbeAssert(result != DXGI_ERROR_DRIVER_INTERNAL_ERROR, "Graphics Device Internal Error");
    bbeAssert(result != DXGI_ERROR_INVALID_CALL, "Graphics Device Invalid Call");
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
    if (SUCCEEDED(Dev()->QueryInterface(IID_PPV_ARGS(&debugDevice1))))
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
    DX12_CALL(Dev()->QueryInterface(__uuidof(ID3D12InfoQueue), (LPVOID*)&debugInfoQueue));

    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, System::GfxDebugLayerBreakOnErrors);
    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, System::GfxDebugLayerBreakOnWarnings);
}

void GfxDevice::EndFrame()
{
    m_CommandListsManager.ExecuteAllActiveCommandLists();
}

void GfxDevice::WaitForPreviousFrame()
{
    bbeProfileFunction();

    // Signal and increment the fence value.
    m_GfxFence.IncrementFenceValue();
    DX12_CALL(m_CommandListsManager.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->Signal(m_GfxFence.Dev(), m_GfxFence.GetFenceValue()));

    if (m_GfxFence.Dev()->GetCompletedValue() < m_GfxFence.GetFenceValue())
    {
        DX12_CALL(m_GfxFence.Dev()->SetEventOnCompletion(m_GfxFence.GetFenceValue(), m_GfxFence.GetFenceEvent()));

        // Dumping profiling blocks may long, so we must wait until the previous frame is finished, else GfxCommandList will try to Reset while executing
        if (SystemProfiler::IsDumpingBlocks())
        {
            WaitForSingleObject(m_GfxFence.GetFenceEvent(), INFINITE);
        }
    }
}

GfxContext GfxDevice::GenerateNewContext(D3D12_COMMAND_LIST_TYPE cmdListType)
{
    GfxContext newContext;
    newContext.m_Device = this;
    newContext.m_CommandList = m_CommandListsManager.Allocate(cmdListType);

    return newContext;
}
