#include <graphic/gfx/gfxadapter.h>
#include <graphic/pch.h>

void GfxAdapter::Initialize()
{
    bbeProfileFunction();

    {
        bbeProfile("CreateDXGIFactory2");
        const UINT factoryFlags = g_CommandLineOptions.m_GfxDebugLayer.m_Enabled ? DXGI_CREATE_FACTORY_DEBUG : 0;
        DX12_CALL(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_DXGIFactory)));
    }

    m_AllAdapters.reserve(4);
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    for (UINT i = 0; m_DXGIFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&hardwareAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        hardwareAdapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't bother with the Basic Render Driver adapter.
            continue;
        }

        g_Log.info("Graphic Adapter found: {}", StringUtils::WideToUtf8(desc.Description));
        m_AllAdapters.push_back(hardwareAdapter);
    }
    assert(m_AllAdapters.empty() == false);
}

void GfxAdapter::Shutdown()
{
    m_AllAdapters.clear();
    m_DXGIFactory.Reset();
}
