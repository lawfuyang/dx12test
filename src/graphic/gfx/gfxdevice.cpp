#include <graphic/gfx/gfxdevice.h>
#include <graphic/pch.h>

static void ConfigureDebugLayerBeforeDeviceCreation()
{
    if (!g_CommandLineOptions.m_GfxDebugLayer.m_Enabled)
        return;

    bbeProfileFunction();
    g_Log.info("Enabling D3D12 Debug Layer");

    ComPtr<ID3D12Debug> debugInterface;
    ComPtr<ID3D12Debug3> debugInterface3;

    DX12_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    DX12_CALL(debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface3)));

    debugInterface3->EnableDebugLayer();
    debugInterface3->SetEnableGPUBasedValidation(g_CommandLineOptions.m_GfxDebugLayer.m_EnableGPUValidation);

    if (!g_CommandLineOptions.m_GfxDebugLayer.m_EnableGPUResourceValidation)
        debugInterface3->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING);

    // TODO: This strict-ness of GfxFence usage when this is enabled is ridiculous! Fix the app, or completely ignore this validation
    debugInterface3->SetEnableSynchronizedCommandQueueValidation(false);

    ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
    DX12_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings)));

    // Turn on auto-breadcrumbs and page fault reporting.
    pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
}

void GfxDevice::Initialize()
{
    bbeProfileFunction();

    ConfigureDebugLayerBeforeDeviceCreation();

    const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_1_0_CORE
    };

    for (D3D_FEATURE_LEVEL level : featureLevels)
    {
        bbeProfile("D3D12CreateDevice");

        m_D3DDevice.Reset();

        // Just take the first device that supports DX12
        if (SUCCEEDED(D3D12CreateDevice(GfxAdapter::GetInstance().GetAllAdapters()[0].Get(), level, IID_PPV_ARGS(&m_D3DDevice))))
        {
            g_Log.info("Created ID3D12Device. D3D_FEATURE_LEVEL: {}", GetD3DFeatureLevelName(level));
            break;
        }
    }
    assert(m_D3DDevice);

    ConfigureDebugLayerAfterDeviceCreation();

    CheckFeaturesSupports();
}

void GfxDevice::CheckStatus()
{
    const ::HRESULT result = m_D3DDevice->GetDeviceRemovedReason();

    if (result == DXGI_ERROR_DEVICE_REMOVED)
        DeviceRemovedHandler();

    assert(result != DXGI_ERROR_DEVICE_HUNG);
    assert(result != DXGI_ERROR_DEVICE_REMOVED);
    assert(result != DXGI_ERROR_DEVICE_RESET);
    assert(result != DXGI_ERROR_DRIVER_INTERNAL_ERROR);
    assert(result != DXGI_ERROR_INVALID_CALL);
}

void GfxDevice::ConfigureDebugLayerAfterDeviceCreation()
{
    if (!g_CommandLineOptions.m_GfxDebugLayer.m_Enabled)
        return;

    bbeProfileFunction();

    ComPtr<ID3D12DebugDevice1> debugDevice1;
    DX12_CALL(Dev()->QueryInterface(IID_PPV_ARGS(&debugDevice1)));

    D3D12_DEBUG_FEATURE debugFeatures = D3D12_DEBUG_FEATURE_NONE;
    if (g_CommandLineOptions.m_GfxDebugLayer.m_EnableConservativeResourceStateTracking)
        debugFeatures |= D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING;
    if (g_CommandLineOptions.m_GfxDebugLayer.m_BehaviorChangingAids)
        debugFeatures |= D3D12_DEBUG_FEATURE_ALLOW_BEHAVIOR_CHANGING_DEBUG_AIDS;

    debugDevice1->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_FEATURE_FLAGS, &debugFeatures, sizeof(debugFeatures));

    if (g_CommandLineOptions.m_GfxDebugLayer.m_EnableGPUValidation)
    {
        const D3D12_DEBUG_DEVICE_GPU_BASED_VALIDATION_SETTINGS gpuValidationSettings
        {
            256,
            D3D12_GPU_BASED_VALIDATION_SHADER_PATCH_MODE_GUARDED_VALIDATION, // erroneous instructions are skipped in execution
            D3D12_GPU_BASED_VALIDATION_PIPELINE_STATE_CREATE_FLAG_NONE       // on the fly creation of patch PSO
        };
        debugDevice1->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_GPU_BASED_VALIDATION_SETTINGS, &gpuValidationSettings, sizeof(gpuValidationSettings));
    }

    // Filter unwanted gfx warnings/errors
    ComPtr<ID3D12InfoQueue> debugInfoQueue;
    DX12_CALL(Dev()->QueryInterface(__uuidof(ID3D12InfoQueue), (LPVOID*)&debugInfoQueue));

    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, g_CommandLineOptions.m_GfxDebugLayer.m_BreakOnWarnings);
    debugInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, g_CommandLineOptions.m_GfxDebugLayer.m_BreakOnErrors);

    D3D12_MESSAGE_ID warningsToIgnore[] =
    {
        //These ID3D12PipelineLibrary errors are handled in code, no need to break on these warnings in debug layer
        D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND,
        D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_DRIVERVERSIONMISMATCH,
        D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_ADAPTERVERSIONMISMATCH,
        D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_UNSUPPORTED,

        // D3D12MA causes this
        //D3D12_MESSAGE_ID_HEAP_ADDRESS_RANGE_INTERSECTS_MULTIPLE_BUFFERS
    };
    D3D12_INFO_QUEUE_FILTER ignoreWarningsFilter{};
    ignoreWarningsFilter.DenyList.NumIDs = _countof(warningsToIgnore);
    ignoreWarningsFilter.DenyList.pIDList = warningsToIgnore;
    debugInfoQueue->AddStorageFilterEntries(&ignoreWarningsFilter);
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

    for (D3D12_FEATURE_DATA_SHADER_MODEL shaderModel : gs_AllD3D12ShaderModels)
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
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &m_D3D12Options5, sizeof(m_D3D12Options5)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &m_D3D12Options6, sizeof(m_D3D12Options6)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &m_D3D12Options7, sizeof(m_D3D12Options7)));
    DX12_CALL(m_D3DDevice->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &m_D3D12GPUVirtualAddressSupport, sizeof(m_D3D12GPUVirtualAddressSupport)));
}

BBE_OPTIMIZE_OFF;
void DeviceRemovedHandler()
{
    g_Log.critical("Device removed!");

    if (!g_CommandLineOptions.m_GfxDebugLayer.m_Enabled)
        return;

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    ComPtr<ID3D12DeviceRemovedExtendedData> pDred;
    DX12_CALL(gfxDevice.Dev()->QueryInterface(IID_PPV_ARGS(&pDred)));

    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT DredAutoBreadcrumbsOutput{};
    D3D12_DRED_PAGE_FAULT_OUTPUT DredPageFaultOutput{};
    DX12_CALL(pDred->GetAutoBreadcrumbsOutput(&DredAutoBreadcrumbsOutput));
    DX12_CALL(pDred->GetPageFaultAllocationOutput(&DredPageFaultOutput));

    // Custom processing of DRED data can be done here.
    // Produce telemetry...
    // Log information to console...
    // break into a debugger...

    assert(false);
}
BBE_OPTIMIZE_ON;
