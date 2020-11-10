#pragma once

#include <graphic/renderers/gfxrendererbase.h>

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxContext;
class GfxRootSignature;
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
    void InitFontsTexture();
    void GrowBuffers(const IMGUIDrawData& imguiDrawData, GfxContext* context = nullptr);
    void UploadBufferData(const IMGUIDrawData& imguiDrawData);
    void SetupRenderStates(GfxContext&, const IMGUIDrawData& imguiDrawData);

    GfxRootSignature* m_RootSignature = nullptr;
    GfxVertexBuffer   m_VertexBuffer;
    GfxIndexBuffer    m_IndexBuffer;
    GfxTexture        m_FontsTexture;
};
#define g_GfxIMGUIRenderer GfxIMGUIRenderer::GetInstance()
