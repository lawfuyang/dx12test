#include "gfxswapchain.h"

#include "graphic/gfxadapter.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

extern ::HWND g_EngineWindowHandle;

void GfxSwapChain::Initialize(GfxDevice* gfxDevice, uint32_t width, uint32_t height, DXGI_FORMAT format)
{
    bbeProfileFunction();

    bbeAssert(m_OwnerDevice == nullptr, "");
    m_OwnerDevice = gfxDevice;

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = format;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    const auto dxgiFactory = GfxAdapter::GetInstance().GetDXGIFactory();

    ComPtr<IDXGISwapChain1> swapChain;
    DX12_CALL(dxgiFactory->CreateSwapChainForHwnd(
        m_OwnerDevice->GetCommandQueue().Dev(),        // Swap chain needs the queue so that it can force a flush on it.
        g_EngineWindowHandle,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain));

    // Disable Alt-Enter and other DXGI trickery...
    DX12_CALL(dxgiFactory->MakeWindowAssociation(g_EngineWindowHandle, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));

    DX12_CALL(swapChain.As(&m_SwapChain));

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
}
