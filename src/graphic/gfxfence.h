#pragma once

class GfxDevice;

class GfxFence
{
public:
    ~GfxFence()
    {
        ::CloseHandle(m_FenceEvent);
    }

    ID3D12Fence* Dev() const { return m_Fence.Get(); }

    void Initialize(D3D12_FENCE_FLAGS);

    ::HANDLE GetFenceEvent() const { return m_FenceEvent; }

    uint64_t GetFenceValue() const { return m_FenceValue; }
    void IncrementFenceValue() { ++m_FenceValue; }

private:
    ComPtr<ID3D12Fence> m_Fence;

    uint64_t m_FenceValue = 0;
    ::HANDLE m_FenceEvent = nullptr;
};
