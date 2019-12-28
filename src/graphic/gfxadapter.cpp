#include "graphic/gfxadapter.h"

#include "graphic/dx12utils.h"

void GfxAdapter::Initialize()
{
    bbeProfileFunction();

    {
        bbeProfile("CreateDXGIFactory2");
        const UINT factoryFlags = g_EnableGfxDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0;
        DX12_CALL(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_DXGIFactory)));
    }

    m_AllAdapters.reserve(4);
    ComPtr<IDXGIAdapter1> hardwareAdapter;
    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != m_DXGIFactory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        hardwareAdapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't bother with the Basic Render Driver adapter.
            continue;
        }

        g_Log.info("Graphic Adapter found: {}", utf8_encode(desc.Description).c_str());
        m_AllAdapters.push_back(hardwareAdapter);
    }

    assert(m_AllAdapters.empty() == false);
}
