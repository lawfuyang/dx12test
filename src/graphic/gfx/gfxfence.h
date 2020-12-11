#pragma once

class GfxFence
{
public:
    ~GfxFence()
    {
        ::CloseHandle(m_FenceEvent);
    }

    D3D12Fence* Dev() const { return m_Fence.Get(); }

    void Initialize();
    void IncrementAndSignal(ID3D12CommandQueue* cmdQueue);
    void WaitForSignalFromGPU() const;

    uint64_t GetValue() const { return m_FenceValue; }
    ::HANDLE GetEvent() const { return m_FenceEvent; }

private:
    ComPtr<D3D12Fence> m_Fence;

    uint64_t m_FenceValue = 0;
    ::HANDLE m_FenceEvent = nullptr;
};
