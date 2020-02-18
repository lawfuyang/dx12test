#pragma once

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;

class GfxSwapChain
{
public:
    static const uint32_t ms_NumFrames = 2;

    IDXGISwapChain4* Dev() const { return m_SwapChain.Get(); }

    GfxTexture& GetCurrentBackBuffer() { return m_RenderTargets[m_FrameIndex]; }

    void Initialize();
    void TransitionBackBufferForPresent(GfxContext&);
    void Present();

private:
    ComPtr<IDXGISwapChain4> m_SwapChain;
    GfxTexture m_RenderTargets[ms_NumFrames];

    uint32_t m_FrameIndex = 0;
};
