#pragma once

class GfxDevice;

enum class GfxQueueType
{
    Graphics,
    Compute,
    Copy,
    NumQueueType,
};

class GfxCommandQueue
{
public:
    ID3D12CommandQueue* Dev() const { return m_CommandQueue.Get(); }

    void Initialize(GfxDevice*, D3D12_COMMAND_LIST_TYPE, D3D12_COMMAND_QUEUE_FLAGS);

private:
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    GfxDevice* m_OwnerDevice = nullptr;
};
