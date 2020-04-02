#pragma once

#include "graphic/renderpasses/gfxrenderpass.h"

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;
struct IMGUIDrawData;

class GfxIMGUIRenderer : public GfxRenderPass
{
public:
    DeclareSingletonFunctions(GfxIMGUIRenderer);

    void Initialize();
    void ShutDown() override;
    void Render(GfxContext&) override;

private:
    void InitFontsTexture(GfxContext&);
    void GrowBuffers(GfxContext&, const IMGUIDrawData& imguiDrawData);
    void UploadBufferData(const IMGUIDrawData& imguiDrawData);
    void SetupRenderStates(GfxContext&);

    GfxVertexBuffer   m_VertexBuffer;
    GfxIndexBuffer    m_IndexBuffer;
    GfxConstantBuffer m_ConstantBuffer;
    GfxTexture        m_FontsTexture;
};
#define g_GfxIMGUIRenderer GfxIMGUIRenderer::GetInstance()
