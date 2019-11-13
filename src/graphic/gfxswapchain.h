#pragma once

#include "graphic/gfxdescriptorheap.h"
#include "graphic/gfxtextures.h"

class GfxDevice;

class GfxSwapChain
{
public:
    static const uint32_t ms_NumFrames = 2;

    void Initialize(uint32_t width, uint32_t height, DXGI_FORMAT);

private:
    ComPtr<IDXGISwapChain4> m_SwapChain;
    GfxRenderTargetView m_RenderTargets[ms_NumFrames];

    uint32_t m_FrameIndex = 0;
};
