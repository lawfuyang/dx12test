#pragma once

class GfxAdapter
{
    DeclareSingletonFunctions(GfxAdapter);

public:
    using GfxAdapterArray = InplaceArray<ComPtr<IDXGIAdapter1>, 2>;

    void Initialize();
    void Shutdown();

    IDXGIFactory7* GetDXGIFactory() const { return m_DXGIFactory.Get(); }
    const GfxAdapterArray& GetAllAdapters() const { return m_AllAdapters; }

private:
    ComPtr<IDXGIFactory7> m_DXGIFactory;
    GfxAdapterArray m_AllAdapters;
};
#define g_GfxAdapter GfxAdapter::GetInstance()
