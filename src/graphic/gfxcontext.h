#pragma once

#include "graphic/gfxpipelinestateobject.h"
#include "graphic/gfxvertexformat.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxmanager.h"
#include "graphic/gfxdevice.h"
#include "graphic/gfxshadermanager.h"

class GfxManager;
class GfxDevice;
class GfxCommandList;
class GfxRenderTargetView;
class GfxPSOManager;
class GfxRootSignature;
class GfxShaderManager;

class GfxContext
{
public:
    void ClearRenderTargetView(GfxRenderTargetView& rtv, XMFLOAT4 clearColor) const;
    void SetRenderTarget(uint32_t idx, GfxRenderTargetView& rtv);

    GfxManager&             GetGfxManager()    { return *m_GfxManager; }
    GfxDevice&              GetDevice()        { return *m_Device; }
    GfxCommandList&         GetCommandList()   { return *m_CommandList; }
    GfxPSOManager&          GetPSOManager()    { return *m_PSOManager; }
    GfxShaderManager&       GetShaderManager() { return *m_ShaderManager; }
    GfxPipelineStateObject& GetPSO()           { return m_PSO; }

    void CompileAndSetPipelineState();

private:
    CD3DX12_VIEWPORT  m_Viewport{ 0.0f, 0.0f, System::APP_WINDOW_WIDTH, System::APP_WINDOW_HEIGHT };
    CD3DX12_RECT      m_ScissorRect{ 0, 0, static_cast<LONG>(System::APP_WINDOW_WIDTH), static_cast<LONG>(System::APP_WINDOW_HEIGHT) };

    GfxManager*          m_GfxManager    = nullptr;
    GfxDevice*           m_Device        = nullptr;
    GfxCommandList*      m_CommandList   = nullptr;
    GfxPSOManager*       m_PSOManager    = nullptr;
    GfxShaderManager*    m_ShaderManager = nullptr;
    GfxRenderTargetView* m_RTVs[8]       = {};

    GfxPipelineStateObject m_PSO;

    friend class GfxDevice;
};
