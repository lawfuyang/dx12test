#pragma once

class GfxManager;
class GfxDevice;
class GfxCommandList;
class GfxRenderTargetView;
class GfxPSOManager;

class GfxContext
{
public:
    void ClearRenderTargetView(GfxRenderTargetView& rtv, XMFLOAT4 clearColor) const;

    GfxManager*     GetGfxManager()  const { return m_GfxManager; }
    GfxDevice*      GetDevice()      const { return m_Device; }
    GfxCommandList* GetCommandList() const { return m_CommandList; }
    GfxPSOManager*  GetPSOManager()  const { return m_PSOManager; }

private:
    GfxManager*     m_GfxManager  = nullptr;
    GfxDevice*      m_Device      = nullptr;
    GfxCommandList* m_CommandList = nullptr;
    GfxPSOManager*  m_PSOManager  = nullptr;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_GraphicPSODesc = {};

    friend class GfxDevice;
};
