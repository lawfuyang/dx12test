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

    GfxPipelineStateObject& pso = context.GetPSO();
    pso.SetVertexInputLayout(gs_Position3f_TexCoord2f);

    GfxShader& vs = context.GetShaderManager().GetShader(AllShaders::VS_TestTriangle);
    GfxShader& ps = context.GetShaderManager().GetShader(AllShaders::PS_TestTriangle);

    pso.SetVertexShader(&vs);
    pso.SetPixelShader(&ps);
}
