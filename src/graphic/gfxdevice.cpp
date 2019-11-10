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
}

void GfxDevice::ShutDown()
{
}
