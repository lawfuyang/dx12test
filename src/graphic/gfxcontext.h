#pragma once

class GfxDevice;
class GfxCommandList;
class GfxRenderTargetView;

class GfxContext
{
public:
    void ClearRenderTargetView(GfxRenderTargetView& rtv, XMFLOAT4 clearColor) const;

    GfxDevice* GetDevice() const { return m_Device; }
    GfxCommandList* GetCommandList() const { return m_CommandList; }

private:
    GfxDevice* m_Device = nullptr;
    GfxCommandList* m_CommandList = nullptr;

    friend class GfxDevice;
};
