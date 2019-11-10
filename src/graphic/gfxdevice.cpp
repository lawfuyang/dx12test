#include "graphic/precomp.h"
#include "graphic/gfxdevice.h"

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
        ID3D12Debug* debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            auto debugControllerPtr = MakeAutoComPtr(debugController);
            debugControllerPtr->EnableDebugLayer();

            ID3D12Debug1* debugController1;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
            {
                auto debugController1Ptr = MakeAutoComPtr(debugController1);
                debugController1Ptr->SetEnableGPUBasedValidation(System::GfxDebugLayerEnableGPUValidation);
                debugController1Ptr->SetEnableSynchronizedCommandQueueValidation(System::GfxDebugLayerSynchronizedCommandQueueValidation);
            }

            ID3D12Debug2* debugController2;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController2))))
            {
                auto debugController2Ptr = MakeAutoComPtr(debugController2);
                debugController2Ptr->SetGPUBasedValidationFlags(System::GfxDebugLayerDisableStateTracking ? D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING : D3D12_GPU_BASED_VALIDATION_FLAGS_NONE);
            }
        }
    }


}

void GfxDevice::ShutDown()
{
}
