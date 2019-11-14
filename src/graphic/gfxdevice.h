#pragma once

#include "graphic/gfxcommandqueue.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxcommandlist.h"
#include "graphic/gfxfence.h"

enum class GfxDeviceType;

class GfxDevice
{
public:
    ID3D12Device* Dev() const { return m_D3DDevice.Get(); }

    ID3D12CommandAllocator* GetCommandAllocator() const { return m_CommandAllocator.Get(); }

    void InitializeForMainDevice();
    const GfxCommandQueue& GetCommandQueue() const { return m_GfxCommandQueue; }

private:
    static void EnableDebugLayer();
    void ConfigureDebugLayer();

    GfxDeviceType m_DeviceType;

    GfxCommandQueue m_GfxCommandQueue;
    GfxCommandList m_CommandList;
    GfxFence m_Fence;

    ComPtr<ID3D12Device6> m_D3DDevice;
    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
};
