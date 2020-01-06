#pragma once

#include "graphic/gfxpipelinestateobject.h"
#include "graphic/gfxvertexformat.h"

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

    GfxManager&       GetGfxManager()    { return *m_GfxManager; }
    GfxDevice&        GetDevice()        { return *m_Device; }
    GfxCommandList&   GetCommandList()   { return *m_CommandList; }
    GfxPSOManager&    GetPSOManager()    { return *m_PSOManager; }
    GfxShaderManager& GetShaderManager() { return *m_ShaderManager; }

    GfxPipelineStateObject& GetPSO() { return m_PSO; }

private:
    GfxManager*       m_GfxManager      = nullptr;
    GfxDevice*        m_Device          = nullptr;
    GfxCommandList*   m_CommandList     = nullptr;
    GfxPSOManager*    m_PSOManager      = nullptr;
    GfxShaderManager* m_ShaderManager   = nullptr;

    GfxPipelineStateObject m_PSO;
    D3D12_INPUT_LAYOUT_DESC m_VertexInputLayout;

    friend class GfxDevice;
};
