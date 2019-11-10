#pragma once

class GfxDevice
{
public:
    GfxDevice();
    ~GfxDevice();

    void InitializeForMainDevice();
    void ShutDown();

private:
    GfxDeviceType m_DeviceType = GfxDeviceType::InvalidDevice;
};
