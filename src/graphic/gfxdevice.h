#pragma once

#include "graphic/gfxcommandqueue.h"
#include "graphic/gfxswapchain.h"

enum class GfxDeviceType;

class GfxDevice
{
public:
    ID3D12Device* Dev() const { return m_D3DDevice.Get(); }

    void InitializeForMainDevice();
    const GfxCommandQueue& GetCommandQueue() const { return m_GfxCommandQueue; }

private:
    static void EnableDebugLayer();
    void ConfigureDebugLayer();

    GfxCommandQueue m_GfxCommandQueue;
    GfxSwapChain m_SwapChain;
    GfxDeviceType m_DeviceType;

    ComPtr<ID3D12Device6> m_D3DDevice;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
};
