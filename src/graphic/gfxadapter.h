#pragma once

extern const bool g_EnableGfxDebugLayer;

struct IDXGIAdapter1;

class GfxAdapter
{
    DeclareSingletonFunctions(GfxAdapter);

public:
    void Initialize();

    IDXGIFactory7* GetDXGIFactory() const { return m_DXGIFactory.Get(); }
    const std::vector<ComPtr<IDXGIAdapter1>>& GetAllAdapters() const { return m_AllAdapters; }

private:
    ComPtr<IDXGIFactory7> m_DXGIFactory;
    std::vector<ComPtr<IDXGIAdapter1>> m_AllAdapters;
};
