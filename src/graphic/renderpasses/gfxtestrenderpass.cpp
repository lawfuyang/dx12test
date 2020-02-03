#include "graphic/renderpasses/gfxtestrenderpass.h"

#include "graphic/gfxcontext.h"

GfxTestRenderPass::GfxTestRenderPass(GfxContext& initContext)
    : GfxRenderPass("GfxTestRenderPass")
{
    initContext.GetCommandList().Dev()->SetName(L"GfxTestRenderPass::GfxTestRenderPass");

    struct Vertex
    {
        XMFLOAT3 m_Position = {};
        XMFLOAT4 m_Color = {};
    };

    // Define the geometry for a triangle.
    const Vertex triangleVertices[] =
    {
        { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
    };

    m_TriangleVBuffer.Initialize(initContext, reinterpret_cast<const void*>(triangleVertices), _countof(triangleVertices), sizeof(Vertex));
}

void GfxTestRenderPass::Render(GfxContext& context)
{
    bbeProfileFunction();

    context.GetCommandList().Dev()->SetName(L"GfxTestRenderPass::Render");

    context.ClearRenderTargetView(context.GetGfxManager().GetSwapChain().GetCurrentBackBuffer(), XMFLOAT4{ 0.0f, 0.2f, 0.4f, 1.0f });

    GfxShaderManager& shaderManager = context.GetShaderManager();

    GfxPipelineStateObject& pso = context.GetPSO();
    pso.SetDepthEnable(false);
    pso.SetStencilEnable(false);
    pso.SetVertexShader(shaderManager.GetShader(ShaderPermutation::VS_TestTriangle));
    pso.SetPixelShader(shaderManager.GetShader(ShaderPermutation::PS_TestTriangle));
    pso.SetVertexInputLayout(GfxDefaultVertexFormats::Position3f_TexCoord2f);

    context.SetVertexBuffer(m_TriangleVBuffer);
    context.SetRenderTarget(0, context.GetGfxManager().GetSwapChain().GetCurrentBackBuffer());

    //context.DrawInstanced(3, 1, 0, 0);
}
