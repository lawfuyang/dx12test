#pragma once

class GfxDevice;

class GfxSwapChain
{
public:
    void Initialize(GfxDevice*, uint32_t width, uint32_t height, DXGI_FORMAT);

private:
    GfxDevice* m_OwnerDevice = nullptr;
    ComPtr<IDXGISwapChain4> m_SwapChain;

    uint32_t m_FrameIndex = 0;
};
