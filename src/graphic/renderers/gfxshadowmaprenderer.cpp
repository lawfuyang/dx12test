#include <graphic/renderers/gfxshadowmaprenderer.h>

#include <system/imguimanager.h>

#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxlightsmanager.h>

static bool gs_ShowGfxShadowMapRendererIMGUIWindow = false;

GfxTexture gs_SunShadowMap;

void GfxShadowMapRenderer::Initialize()
{
    bbeProfileFunction();

    g_IMGUIManager.RegisterTopMenu("Graphic", "GfxShadowMapRenderer", &gs_ShowGfxShadowMapRendererIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUI(); });

    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxShadowMapRenderer::Initialize");

    GfxTexture::InitParams sunShadowMapInitParams;
    sunShadowMapInitParams.m_Format = DXGI_FORMAT_D16_UNORM;
    sunShadowMapInitParams.m_Width = DirectionalLight::ShadowMapResolution;
    sunShadowMapInitParams.m_Height = DirectionalLight::ShadowMapResolution;
    sunShadowMapInitParams.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    sunShadowMapInitParams.m_InitData = nullptr;
    sunShadowMapInitParams.m_ResourceName = "Sun Shadow Map";
    sunShadowMapInitParams.m_InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    sunShadowMapInitParams.m_ViewType = GfxTexture::DSV;

    //gs_SunShadowMap.Initialize(initContext, sunShadowMapInitParams);
}

void GfxShadowMapRenderer::ShutDown()
{
    gs_SunShadowMap.Release();
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
