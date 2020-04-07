#include "graphic/gfxfence.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxFence::Initialize()
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    const UINT64 initialValue = 0;

    // Create ID3D12Fence object
    // TODO: Support other fence flags?
    DX12_CALL(gfxDevice.Dev()->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));

    // Create an event handle for CPU synchronization
    m_FenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(m_FenceEvent);
}

void GfxFence::IncrementAndSignal(ID3D12CommandQueue* cmdQueue)
{
    cmdQueue->Signal(m_Fence.Get(), ++m_FenceValue);
    DX12_CALL(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent));
}

bool GfxFence::IsSignaledByGPU() const
{
    const UINT64 completedValue = m_Fence->GetCompletedValue();
    if (completedValue == UINT64_MAX)
    {
        g_Log.error("Device removed!");
        assert(false);
    }

    assert(completedValue <= m_FenceValue);
    return completedValue == m_FenceValue;
}

void GfxFence::WaitForSignalFromGPU() const
{
    if (!IsSignaledByGPU())
    {
        DX12_CALL(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent));
        WaitForSingleObject(m_FenceEvent, INFINITE);
    }
}
