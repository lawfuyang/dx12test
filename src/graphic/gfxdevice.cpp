#include "graphic/gfxdevice.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxadapter.h"
#include "graphic/gfxenums.h"

void GfxDevice::InitializeForMainDevice()
{
    bbeProfileFunction();

    m_DeviceType = GfxDeviceType::Main;

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

    m_GfxCommandQueue.Initialize(this, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_FLAG_NONE);

    m_GfxFence.Initialize(*this, D3D12_FENCE_FLAG_NONE);
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

void GfxDevice::ClearRenderTargetView(GfxRenderTargetView& rtv, const float(&clearColor)[4])
{
    const UINT numBarriers = 1;

    rtv.GetHazardTrackedResource().Transition(*m_CurrentCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const UINT numRects = 0;
    const D3D12_RECT* pRects = nullptr;

    m_CurrentCommandList->Dev()->ClearRenderTargetView(rtv.GetCPUDescHandle(), clearColor, numRects, pRects);
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

void GfxDevice::ExecuteAllActiveCommandLists()
{
    bbeProfileFunction();

    std::vector<GfxCommandList*> cmdListsToExecute;
    {
        bbeAutoLock(m_ActiveCommandListsLock);
        cmdListsToExecute.swap(m_ActiveCommandLists);
    }

    ID3D12CommandList** ppCommandLists = (ID3D12CommandList**)alloca(cmdListsToExecute.size());
    for (uint32_t i = 0; i < cmdListsToExecute.size(); ++i)
    {
        DX12_CALL(cmdListsToExecute[i]->Dev()->Close());
        ppCommandLists[i] = cmdListsToExecute[i]->Dev();
    }

    m_GfxCommandQueue.Dev()->ExecuteCommandLists(cmdListsToExecute.size(), ppCommandLists);

    {
        bbeAutoLock(m_FreeCommandListsLock);
        m_FreeCommandLists.reserve(cmdListsToExecute.size());
        for (GfxCommandList* gfxCmdList : cmdListsToExecute)
        {
            m_FreeCommandLists.push_back(gfxCmdList);
        }
    }

    m_CurrentCommandList = nullptr;
}

void GfxDevice::WaitForPreviousFrame()
{
    bbeProfileFunction();

    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity

    // Signal and increment the fence value.
    const UINT64 fence = m_GfxFence.GetFenceValue();
    DX12_CALL(m_GfxCommandQueue.Dev()->Signal(m_GfxFence.Dev(), fence));
    m_GfxFence.IncrementFenceValue();

    // Wait until the previous frame is finished.
    if (m_GfxFence.Dev()->GetCompletedValue() < fence)
    {
        DX12_CALL(m_GfxFence.Dev()->SetEventOnCompletion(fence, m_GfxFence.GetFenceEvent()));
        WaitForSingleObject(m_GfxFence.GetFenceEvent(), INFINITE);
    }
}

GfxCommandList* GfxDevice::NewCommandList(const std::string& cmdListName)
{
    bbeProfileFunction();

    GfxCommandList* newCmdList = nullptr;
    auto RenameNewCmdListAndReturn = [&]()
    {
        newCmdList->BeginRecording();
        if (cmdListName.size())
        {
            SetDebugName(newCmdList->Dev(), cmdListName);
        }
        m_CurrentCommandList = newCmdList;
        return newCmdList;
    };

    {
        bbeAutoLock(m_FreeCommandListsLock);
        if (m_FreeCommandLists.size())
        {
            newCmdList = m_FreeCommandLists.back();
            m_FreeCommandLists.pop_back();
            return RenameNewCmdListAndReturn();
        }
    }

    newCmdList = m_CommandListsPool.construct();

    {
        bbeAutoLock(m_ActiveCommandListsLock);
        m_ActiveCommandLists.push_back(newCmdList);
    }

    newCmdList->Initialize(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
    return RenameNewCmdListAndReturn();
}
