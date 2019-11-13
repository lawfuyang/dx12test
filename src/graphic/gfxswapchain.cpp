#include "gfxswapchain.h"

#include "graphic/gfxmanager.h"
#include "graphic/gfxadapter.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

extern ::HWND g_EngineWindowHandle;

void GfxSwapChain::Initialize(uint32_t width, uint32_t height, DXGI_FORMAT format)
{
    bbeProfileFunction();

    bbeAssert(m_SwapChain.Get() == nullptr, "");

    GfxDevice& mainGfxDevice = GfxManager::GetInstance().GetMainGfxDevice();

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = format;
    swapChainDesc.Stereo = false; // set to true for VR
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = ms_NumFrames;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED; // TODO: Learn the differences
    swapChainDesc.SampleDesc.Count = 1; // >1 valid only with bit-block transfer (bitblt) model swap chains.

    const auto dxgiFactory = GfxAdapter::GetInstance().GetDXGIFactory();

    ComPtr<IDXGISwapChain1> swapChain;
    DX12_CALL(dxgiFactory->CreateSwapChainForHwnd(mainGfxDevice.GetCommandQueue().Dev(), // Swap chain needs the queue so that it can force a flush on it.
                                                  g_EngineWindowHandle,
                                                  &swapChainDesc,
                                                  nullptr,
                                                  nullptr,
                                                  &swapChain));

    // Disable Alt-Enter and other DXGI trickery...
    DX12_CALL(dxgiFactory->MakeWindowAssociation(g_EngineWindowHandle, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER));

    DX12_CALL(swapChain.As(&m_SwapChain));

    m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

    m_SwapChainDescHeap.Initialize(&mainGfxDevice, ms_NumFrames, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    // Create swap chain frame resources
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescHandle = m_SwapChainDescHeap.Dev()->GetCPUDescriptorHandleForHeapStart();

    // Create a RTV for each frame.
    for (uint32_t i = 0; i < _countof(m_RenderTargets); ++i)
    {
        DX12_CALL(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_RenderTargets[i])));
        mainGfxDevice.Dev()->CreateRenderTargetView(m_RenderTargets[i].Get(), nullptr, cpuDescHandle);
        cpuDescHandle.ptr += m_SwapChainDescHeap.GetDescSize();
    }
}
