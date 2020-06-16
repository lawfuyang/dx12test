#include <graphic/renderers/gfxshadowmaprenderer.h>

#include <system/imguimanager.h>

#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxcontext.h>

static bool gs_ShowGfxShadowMapRendererIMGUIWindow = false;

void GfxShadowMapRenderer::Initialize()
{
    bbeProfileFunction();
    m_Name = "GfxShadowMapRenderer";

    g_IMGUIManager.RegisterTopMenu("Graphic", "GfxShadowMapRenderer", &gs_ShowGfxShadowMapRendererIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUI(); });

    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxShadowMapRenderer::Initialize");


}

void GfxShadowMapRenderer::ShutDown()
{

}

void GfxShadowMapRenderer::PopulateCommandList()
{

}

void GfxShadowMapRenderer::UpdateIMGUI()
{
    if (!gs_ShowGfxShadowMapRendererIMGUIWindow)
        return;

    ScopedIMGUIWindow window{ "GfxShadowMapRenderer" };
}
