#pragma once

#include "graphic/gfxcommandqueue.h"
#include "graphic/gfxswapchain.h"

enum class GfxDeviceType;

class GfxDevice
{
public:
    GfxDevice();
    ~GfxDevice();

    void InitializeForMainDevice();

    ID3D12Device* Dev() const { return m_D3DDevice.Get(); }
    const GfxCommandQueue& GetCommandQueue() const { return m_GfxCommandQueue; }

private:
    static void EnableDebugLayer();
    void ConfigureDebugLayer();

    ComPtr<ID3D12Device6> m_D3DDevice;
    GfxCommandQueue m_GfxCommandQueue;
    GfxSwapChain m_SwapChain;
    GfxDeviceType m_DeviceType;
};
