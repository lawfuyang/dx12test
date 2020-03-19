#include "graphic/gfxswapchain.h"

#include "graphic/gfxmanager.h"
#include "graphic/gfxadapter.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"
#include "graphic/gfxcontext.h"

void GfxSwapChain::Initialize()
{
    bbeProfileFunction();

    assert(m_SwapChain.Get() == nullptr);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = g_CommandLineOptions.m_WindowWidth;
    swapChainDesc.Height = g_CommandLineOptions.m_WindowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = false; // set to true for VR
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = ms_NumFrames;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // TODO: Learn the differences
    swapChainDesc.SampleDesc.Count = 1; // >1 valid only with bit-block transfer (bitblt) model swap chains.

    const auto dxgiFactory = GfxAdapter::GetInstance().GetDXGIFactory();

    ComPtr<IDXGISwapChain1> swapChain;
    {
        bbeProfile("CreateSwapChainForHwnd");
        DX12_CALL(dxgiFactory->CreateSwapChainForHwnd(gfxDevice.GetCommandListsManager().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT), // Swap chain needs the queue so that it can force a flush on it.
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

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

    // Create a RTV for each frame.
    for (uint32_t i = 0; i < _countof(m_RenderTargets); ++i)
    {
        m_RenderTargets[i].InitializeForSwapChain(m_SwapChain, DXGI_FORMAT_R8G8B8A8_UNORM, i);
    }
}

void GfxSwapChain::ShutDown()
{
    for (GfxTexture& tex : m_RenderTargets)
    {
        tex.Release();
    }
}

void GfxSwapChain::TransitionBackBufferForPresent(GfxContext& context)
{
    bbeProfileFunction();

    m_RenderTargets[m_FrameIndex].Transition(context.GetCommandList(), D3D12_RESOURCE_STATE_PRESENT);
}

void GfxSwapChain::Present()
{
    bbeProfileFunction();

    // TODO: init these values properly
    const UINT syncInterval = 1;
    const UINT flags = 0;

    // Present the frame.
    DX12_CALL(m_SwapChain.Get()->Present(syncInterval, flags));

    m_FrameIndex = m_SwapChain.Get()->GetCurrentBackBufferIndex();
}
