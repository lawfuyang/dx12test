#pragma once

class GfxDevice;
class GfxCommandList;
class GfxRenderTargetView;

class GfxContext
{
public:
    void ClearRenderTargetView(GfxRenderTargetView& rtv, const float(&clearColor)[4]); // TODO: Convert to use math lib's vec4

    GfxDevice* GetDevice() { return m_Device; }
    GfxCommandList* GetCommandList() { return m_CommandList; }

private:
    GfxDevice* m_Device = nullptr;
    GfxCommandList* m_CommandList = nullptr;

    friend class GfxDevice;
};
