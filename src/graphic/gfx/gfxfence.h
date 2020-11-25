#pragma once

class GfxFence
{
public:
    ~GfxFence()
    {
        ::CloseHandle(m_FenceEvent);
    }

    ID3D12Fence1* Dev() const { return m_Fence.Get(); }

    void Initialize();
    void IncrementAndSignal(ID3D12CommandQueue* cmdQueue);
    void WaitForSignalFromGPU() const;

    uint64_t GetValue() const { return m_FenceValue; }
    ::HANDLE GetEvent() const { return m_FenceEvent; }

private:
    ComPtr<ID3D12Fence1> m_Fence;

    uint64_t m_FenceValue = 0;
    ::HANDLE m_FenceEvent = nullptr;
};
