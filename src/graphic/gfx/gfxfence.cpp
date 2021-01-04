#include <graphic/gfx/gfxfence.h>
#include <graphic/pch.h>

void GfxFence::Initialize()
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    const UINT64 initialValue = 0;

    // Create ID3D12Fence object
    // TODO: Support other fence flags?
    DX12_CALL(gfxDevice.Dev()->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));

    // Create an event handle for CPU synchronization
    const bool ManualReset = true;
    const bool InitialState = false;
    m_FenceEvent = ::CreateEvent(nullptr, ManualReset, InitialState, nullptr);
    assert(m_FenceEvent);
}

void GfxFence::IncrementAndSignal(ID3D12CommandQueue* cmdQueue)
{
    assert(m_FenceEvent);

    DX12_CALL(cmdQueue->Signal(m_Fence.Get(), ++m_FenceValue));

    ::ResetEvent(m_FenceEvent);
    DX12_CALL(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent));
}

void GfxFence::WaitForSignalFromGPU() const
{
    assert(m_FenceEvent);

    // check for device removal via returned frame value
    const UINT64 completedValue = m_Fence->GetCompletedValue();
    if (completedValue == UINT64_MAX)
        DeviceRemovedHandler();
    assert(completedValue <= m_FenceValue);

    if (completedValue == m_FenceValue)
        return;

    bbeProfileFunction();

    const DWORD waitResult = ::WaitForSingleObject(m_FenceEvent, 1000);

    // GPU hang? Deadlock?
    // Debug layer held back actual submission of GPU work until all outstanding fence Wait conditions are met? See: ID3D12Debug3::SetEnableSynchronizedCommandQueueValidation
    assert(waitResult != WAIT_TIMEOUT);
}
