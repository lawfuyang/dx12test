#include "graphic/gfxdevice.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxadapter.h"
#include "graphic/gfxcontext.h"
#include "graphic/gfxdescriptorheap.h"
#include "graphic/gfxrootsignature.h"
#include "graphic/gfxpipelinestateobject.h"

const bool            g_EnableGfxDebugLayer                     = true;
static constexpr bool gs_BreakOnWarnings                        = g_EnableGfxDebugLayer && true;
static constexpr bool gs_BreakOnErrors                          = g_EnableGfxDebugLayer && true;
static constexpr bool gs_LogVerbose                             = g_EnableGfxDebugLayer && true;
static constexpr bool gs_EnableGPUValidation                    = g_EnableGfxDebugLayer && false; // TODO: Re-enable when crash from 441.66 is fixed
static constexpr bool gs_SynchronizedCommandQueueValidation     = g_EnableGfxDebugLayer && true;
static constexpr bool gs_DisableStateTracking                   = g_EnableGfxDebugLayer && false;
static constexpr bool gs_EnableConservativeResorceStateTracking = g_EnableGfxDebugLayer && true;

void GfxDevice::Initialize()
{
    bbeProfileFunction();

    if constexpr (g_EnableGfxDebugLayer)
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

        // Just take the first device that supports DX12
        if (SUCCEEDED(D3D12CreateDevice(GfxAdapter::GetInstance().GetAllAdapters()[0].Get(), level, IID_PPV_ARGS(&m_D3DDevice))))
        {
            g_Log.info("Created ID3D12Device. D3D_FEATURE_LEVEL: {}", GetD3DFeatureLevelName(level));
            break;
        }
    }
    assert(m_D3DDevice.Get() != nullptr);

    if constexpr (g_EnableGfxDebugLayer)
    {
        ConfigureDebugLayer();
    }

    m_GfxFence.Initialize(D3D12_FENCE_FLAG_NONE);

    CheckFeaturesSupports();

    m_AllContexts.reserve(16);
}

void GfxDevice::CheckStatus()
{
    const ::HRESULT result = m_D3DDevice->GetDeviceRemovedReason();

    assert(result != DXGI_ERROR_DEVICE_HUNG);
    assert(result != DXGI_ERROR_DEVICE_REMOVED);
    assert(result != DXGI_ERROR_DEVICE_RESET);
    assert(result != DXGI_ERROR_DRIVER_INTERNAL_ERROR);
    assert(result != DXGI_ERROR_INVALID_CALL);
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
            debugController1->SetEnableGPUBasedValidation(gs_EnableGPUValidation);
            debugController1->SetEnableSynchronizedCommandQueueValidation(gs_SynchronizedCommandQueueValidation);
        }

        ComPtr<ID3D12Debug2> debugController2;
        if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController2))))
        {
            debugController2->SetGPUBasedValidationFlags(gs_DisableStateTracking ? D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING : D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
        }
    }
}

void GfxDevice::ConfigureDebugLayer()
{
    bbeProfileFunction();

    ComPtr<ID3D12DebugDevice1> debugDevice1;
    if (SUCCEEDED(Dev()->QueryInterface(IID_PPV_ARGS(&debugDevice1))))
    {
        if (gs_EnableConservativeResorceStateTracking)
        {
            const D3D12_DEBUG_FEATURE debugFeatures
            {
                D3D12_DEBUG_FEATURE_ALLOW_BEHAVIOR_CHANGING_DEBUG_AIDS | D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING
            };
            debugDevice1->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_FEATURE_FLAGS, &debugFeatures, sizeof(debugFeatures));
        }

        if (gs_EnableGPUValidation)
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

    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, gs_BreakOnErrors);
    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, gs_BreakOnWarnings);
}

void GfxDevice::CheckFeaturesSupports()
{
    bbeProfileFunction();

    IDXGIFactory7* dxgiFactory = GfxAdapter::GetInstance().GetDXGIFactory();

    // tearing, or adaptive sync
    BOOL tearingSupported;
    DX12_CALL(dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupported, sizeof(tearingSupported)));
    m_TearingSupported = tearingSupported;

    // D3D_ROOT_SIGNATURE_VERSION_1_1 is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
    if (FAILED(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &m_RootSigSupport, sizeof(m_RootSigSupport))))
    {
        m_RootSigSupport.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Query the level of support of Shader Model.
    constexpr D3D12_FEATURE_DATA_SHADER_MODEL shaderModels[] = 
    {
        D3D_SHADER_MODEL_6_5,
        D3D_SHADER_MODEL_6_4,
        D3D_SHADER_MODEL_6_3,
        D3D_SHADER_MODEL_6_2,
        D3D_SHADER_MODEL_6_1,
        D3D_SHADER_MODEL_6_0
    };
    for (D3D12_FEATURE_DATA_SHADER_MODEL shaderModel : shaderModels)
    {
        if (SUCCEEDED(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))))
        {
            g_Log.info("D3D12_FEATURE_SHADER_MODEL support: {}", GetD3DShaderModelName(shaderModel.HighestShaderModel));
            m_D3DHighestShaderModel.HighestShaderModel = shaderModel.HighestShaderModel;
            break;
        }
    }
    assert(m_D3DHighestShaderModel.HighestShaderModel != D3D_SHADER_MODEL_5_1);

    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &m_D3D12Options, sizeof(m_D3D12Options)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &m_D3D12Options1, sizeof(m_D3D12Options1)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &m_D3D12Options2, sizeof(m_D3D12Options2)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &m_D3D12Options3, sizeof(m_D3D12Options3)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &m_D3D12Options4, sizeof(m_D3D12Options4)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &m_D3D12Options5, sizeof(m_D3D12Options4)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &m_D3D12GPUVirtualAddressSupport, sizeof(m_D3D12GPUVirtualAddressSupport)));
}

void GfxDevice::EndFrame()
{
    bbeProfileFunction();

    m_CommandListsManager.ExecuteAllActiveCommandLists();
    m_AllContexts.clear();
}

void GfxDevice::WaitForPreviousFrame()
{
    bbeProfileFunction();

    m_GfxFence.IncrementAndSignal(m_CommandListsManager.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT));
    m_GfxFence.WaitForSignalFromGPU();
}

GfxContext& GfxDevice::GenerateNewContext(D3D12_COMMAND_LIST_TYPE cmdListType)
{
    bbeProfileFunction();

    m_AllContexts.push_back(GfxContext{});
    GfxContext& newContext = m_AllContexts.back();
    newContext.m_GfxManager = &GfxManager::GetInstance();
    newContext.m_Device = this;
    newContext.m_CommandList = m_CommandListsManager.Allocate(cmdListType);

    // Set default gfx root sig
    newContext.m_PSO.SetRootSignature(&g_DefaultGraphicsRootSignature);

    return newContext;
}
