#pragma once

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;

class GfxSwapChain
{
public:
    IDXGISwapChain4* Dev() const { return m_SwapChain.Get(); }

    GfxTexture& GetCurrentBackBuffer() { return m_Textures[Dev()->GetCurrentBackBufferIndex()]; }

    void Initialize();
    void Present();

private:
    static const uint32_t NbBackBuffers = 2;

    ComPtr<IDXGISwapChain4> m_SwapChain;
    GfxTexture m_Textures[NbBackBuffers];
};
