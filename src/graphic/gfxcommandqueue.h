#pragma once

class GfxDevice;

class GfxCommandQueue
{
public:
    void Initialize(GfxDevice*, D3D12_COMMAND_LIST_TYPE);

    ID3D12CommandQueue* Dev() const { return m_CommandQueue.Get(); }

private:
    ComPtr<ID3D12CommandQueue> m_CommandQueue;
    GfxDevice* m_OwnerDevice = nullptr;
};
