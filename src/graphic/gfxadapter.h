#pragma once

extern const bool g_EnableGfxDebugLayer;

using GfxAdapterArray = boost::container::small_vector<ComPtr<IDXGIAdapter1>, 4>;

class GfxAdapter
{
    DeclareSingletonFunctions(GfxAdapter);

public:
    void Initialize();

    IDXGIFactory7* GetDXGIFactory() const { return m_DXGIFactory.Get(); }
    const GfxAdapterArray& GetAllAdapters() const { return m_AllAdapters; }

private:
    ComPtr<IDXGIFactory7> m_DXGIFactory;
    GfxAdapterArray m_AllAdapters;
};
