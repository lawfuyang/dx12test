#include <graphic/renderers/gfxzprepassrenderer.h>

#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxdefaultassets.h>

#include <tmp/shaders/autogen/cpp/PerFrameConsts.h>
#include <tmp/shaders/autogen/cpp/VS_ForwardLighting.h>
#include <tmp/shaders/autogen/cpp/PS_ForwardLighting.h>

extern GfxTexture gs_SceneDepthBuffer;

void GfxZPrePassRenderer::Initialize()
{
    bbeProfileFunction();
}

void GfxZPrePassRenderer::ShutDown()
{

}

void GfxZPrePassRenderer::PopulateCommandList()
{
    bbeProfileFunction();

}
