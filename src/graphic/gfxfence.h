#pragma once

class GfxDevice;

class GfxFence
{
public:
    void Initialize(GfxDevice& gfxDevice, D3D12_FENCE_FLAGS);

private:
    ComPtr<ID3D12Fence> m_Fence;

    uint64_t m_FenceValue = 1;
    ::HANDLE m_FenceEvent = nullptr;
};
