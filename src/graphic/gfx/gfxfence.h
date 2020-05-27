#pragma once

class GfxDevice;

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
    bool IsSignaledByGPU() const;
    void WaitForSignalFromGPU() const;

private:
    ComPtr<ID3D12Fence1> m_Fence;

    uint64_t m_FenceValue = 0;
    ::HANDLE m_FenceEvent = nullptr;
};
