#pragma once

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;

class GfxSwapChain
{
public:
    static const uint32_t ms_NumFrames = 2;

    IDXGISwapChain4* Dev() const { return m_SwapChain.Get(); }

    GfxRenderTargetView& GetCurrentBackBuffer() { return m_RenderTargets[m_FrameIndex]; }

    void Initialize();
    void ShutDown();
    void TransitionBackBufferForPresent(GfxContext&);
    void Present();

private:
    ComPtr<IDXGISwapChain4> m_SwapChain;
    GfxRenderTargetView m_RenderTargets[ms_NumFrames];

    uint32_t m_FrameIndex = 0;
};
