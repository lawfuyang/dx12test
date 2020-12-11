#include <graphic/gfx/gfxswapchain.h>
#include <graphic/pch.h>

void GfxSwapChain::Initialize()
{
    bbeProfileFunction();

    assert(m_SwapChain.Get() == nullptr);

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = g_CommandLineOptions.m_WindowWidth;
    swapChainDesc.Height = g_CommandLineOptions.m_WindowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = false; // set to true for VR
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = NbBackBuffers;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // TODO: Learn the differences
    swapChainDesc.SampleDesc.Count = 1; // >1 valid only with bit-block transfer (bitblt) model swap chains.

    const auto dxgiFactory = GfxAdapter::GetInstance().GetDXGIFactory();

    ComPtr<IDXGISwapChain1> swapChain;
    {
        bbeProfile("CreateSwapChainForHwnd");
        DX12_CALL(dxgiFactory->CreateSwapChainForHwnd(g_GfxCommandListsManager.GetMainQueue().Dev(), // Swap chain needs the queue so that it can force a flush on it.
                                                      g_System.GetEngineWindowHandle(),
                                                      &swapChainDesc,
                                                      nullptr,
                                                      nullptr,
                                                      &swapChain));
    }

    // Disable Alt-Enter and other DXGI trickery...
    DX12_CALL(dxgiFactory->MakeWindowAssociation(g_System.GetEngineWindowHandle(), DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));

    DX12_CALL(swapChain.As(&m_SwapChain));
    swapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain));

    // set debug names
    for (uint32_t i = 0; i < NbBackBuffers; ++i)
    {
        D3D12Resource* swapChainResource = nullptr;
        DX12_CALL(Dev()->GetBuffer(i, IID_PPV_ARGS(&swapChainResource)));
        m_Textures[i].m_D3D12Resource = swapChainResource;
        m_Textures[i].m_D3D12Resource->SetName(StringUtils::Utf8ToWide(StringFormat("Back Buffer RTV %d", i)));
        m_Textures[i].m_Format = swapChainDesc.Format;
    }
}

void GfxSwapChain::Present()
{
    bbeProfileFunction();

    const UINT syncInterval = 0; // Need to sync CPU frame to this function if we want V-SYNC
    const UINT flags = 0;

    // Present the frame.
    DX12_CALL(m_SwapChain.Get()->Present(syncInterval, flags));
}
