#pragma once

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;

class GfxSwapChain
{
public:
    DXGISwapChain* Dev() const { return m_SwapChain.Get(); }

    GfxTexture& GetCurrentBackBuffer() { return m_Textures[Dev()->GetCurrentBackBufferIndex()]; }

    void Initialize();
    void Shutdown() { m_SwapChain.Reset(); }
    void Present();

private:
    static const uint32_t NbBackBuffers = 2;

    ComPtr<DXGISwapChain> m_SwapChain;
    GfxTexture m_Textures[NbBackBuffers];
};
