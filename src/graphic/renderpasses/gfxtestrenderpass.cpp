#include "graphic/renderpasses/gfxtestrenderpass.h"

#include "graphic/gfxcontext.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxmanager.h"
#include "graphic/gfxdevice.h"
#include "graphic/gfxshadermanager.h"

GfxTestRenderPass::GfxTestRenderPass()
    : GfxRenderPass("GfxTestRenderPass")
{
}

void GfxTestRenderPass::Render(GfxContext& context)
{
    bbeProfileFunction();

    GfxShaderManager& shaderManager = context.GetShaderManager();

    GfxPipelineStateObject& pso = context.GetPSO();
    pso.SetVertexInputLayout(GfxDefaultVertexFormats::Position3f_TexCoord2f);
    pso.SetDepthEnable(false);
    pso.SetStencilEnable(false);
    pso.SetRenderTargetFormat(0, DXGI_FORMAT_R8G8B8A8_UNORM);
    pso.SetVertexShader(shaderManager.GetShader(AllShaders::VS_TestTriangle));
    pso.SetPixelShader(shaderManager.GetShader(AllShaders::PS_TestTriangle));
}
