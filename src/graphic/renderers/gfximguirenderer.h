#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;
struct IMGUIDrawData;

class GfxIMGUIRenderer : public GfxRendererBase
{
public:
    DeclareSingletonFunctions(GfxIMGUIRenderer);

    void Initialize();
    void ShutDown() override;
    void PopulateCommandList() override;

    const char* GetName() const override { return "GfxIMGUIRenderer"; }

private:
    void InitFontsTexture(GfxContext&);
    void GrowBuffers(GfxContext&, const IMGUIDrawData& imguiDrawData);
    void UploadBufferData(const IMGUIDrawData& imguiDrawData);
    void SetupRenderStates(GfxContext&, const IMGUIDrawData& imguiDrawData);

    GfxRootSignature  m_RootSignature;
    GfxVertexBuffer   m_VertexBuffer;
    GfxIndexBuffer    m_IndexBuffer;
    GfxTexture        m_FontsTexture;
};
#define g_GfxIMGUIRenderer GfxIMGUIRenderer::GetInstance()
