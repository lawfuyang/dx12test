#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;
struct IMGUIDrawData;

class GfxIMGUIRenderer : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxIMGUIRenderer);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList(GfxContext&) override;

private:
    void InitFontsTexture(GfxContext&);
    void GrowBuffers(GfxContext&, const IMGUIDrawData& imguiDrawData);
    void UploadBufferData(const IMGUIDrawData& imguiDrawData);
    void SetupRenderStates(GfxContext&, const IMGUIDrawData& imguiDrawData);

    GfxVertexBuffer   m_VertexBuffer;
    GfxIndexBuffer    m_IndexBuffer;
    GfxConstantBuffer m_ConstantBuffer;
    GfxTexture        m_FontsTexture;
};
#define g_GfxIMGUIRenderer GfxIMGUIRenderer::GetInstance()
