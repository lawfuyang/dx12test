#include "graphic/renderpasses/gfxtestrenderpass.h"

#include "graphic/gfxcontext.h"

GfxTestRenderPass::GfxTestRenderPass(GfxContext& initContext)
    : GfxRenderPass("GfxTestRenderPass")
{
    initContext.GetCommandList().Dev()->SetName(L"GfxTestRenderPass::GfxTestRenderPass");

    struct Vertex
    {
        XMFLOAT3 m_Position = {};
        XMFLOAT2 m_TexCoord = {};
        uint32_t m_Color = 0xFF0000FF;
    };

    // Define the geometry for the cube
    const Vertex vList[] =
    {
        // front face
        { { -0.5f,  0.5f, -0.5f }, { 0.f, 0.f }, 0xFF0000FF },
        { {  0.5f, -0.5f, -0.5f }, { 1.f, 1.f }, 0xFF0000FF },
        { { -0.5f, -0.5f, -0.5f }, { 0.f, 1.f }, 0xFF0000FF },
        { {  0.5f,  0.5f, -0.5f }, { 1.f, 0.f }, 0xFF0000FF },

        // right side face
        { {  0.5f, -0.5f, -0.5f }, { 0.f, 1.f }, 0xFF00FF00 },
        { {  0.5f,  0.5f,  0.5f }, { 1.f, 0.f }, 0xFF00FF00 },
        { {  0.5f, -0.5f,  0.5f }, { 1.f, 1.f }, 0xFF00FF00 },
        { {  0.5f,  0.5f, -0.5f }, { 0.f, 0.f }, 0xFF00FF00 },

        // left side face
        { { -0.5f,  0.5f,  0.5f }, { 0.f, 0.f }, 0xFFFF0000 },
        { { -0.5f, -0.5f, -0.5f }, { 1.f, 1.f }, 0xFFFF0000 },
        { { -0.5f, -0.5f,  0.5f }, { 0.f, 1.f }, 0xFFFF0000 },
        { { -0.5f,  0.5f, -0.5f }, { 1.f, 0.f }, 0xFFFF0000 },

        // back face
        { {  0.5f,  0.5f,  0.5f }, { 0.f, 0.f }, 0xFF00FFFF },
        { { -0.5f, -0.5f,  0.5f }, { 1.f, 1.f }, 0xFF00FFFF },
        { {  0.5f, -0.5f,  0.5f }, { 0.f, 1.f }, 0xFF00FFFF },
        { { -0.5f,  0.5f,  0.5f }, { 1.f, 0.f }, 0xFF00FFFF },

        // top face
        { { -0.5f,  0.5f, -0.5f }, { 0.f, 0.f }, 0xFFFF00FF },
        { {  0.5f,  0.5f,  0.5f }, { 1.f, 1.f }, 0xFFFF00FF },
        { {  0.5f,  0.5f, -0.5f }, { 0.f, 1.f }, 0xFFFF00FF },
        { { -0.5f,  0.5f,  0.5f }, { 1.f, 0.f }, 0xFFFF00FF },

        // bottom face
        { {  0.5f, -0.5f,  0.5f }, { 0.f, 0.f }, 0xFFFFFF00 },
        { { -0.5f, -0.5f, -0.5f }, { 1.f, 1.f }, 0xFFFFFF00 },
        { {  0.5f, -0.5f, -0.5f }, { 0.f, 1.f }, 0xFFFFFF00 },
        { { -0.5f, -0.5f,  0.5f }, { 1.f, 0.f }, 0xFFFFFF00 },
    };

    // a quad (2 triangles)
    const uint16_t iList[] = {
        // front face
        0, 1, 2, // first triangle
        0, 3, 1, // second triangle

        // left face
        4, 5, 6, // first triangle
        4, 7, 5, // second triangle

        // right face
        8, 9, 10, // first triangle
        8, 11, 9, // second triangle

        // back face
        12, 13, 14, // first triangle
        12, 15, 13, // second triangle

        // top face
        16, 17, 18, // first triangle
        16, 19, 17, // second triangle

        // bottom face
        20, 21, 22, // first triangle
        20, 23, 21, // second triangle
    };

    D3D12MA::Allocation* alloc = m_QuadVBuffer.Initialize(initContext, vList, _countof(vList), sizeof(Vertex));
    alloc->SetName(L"GfxTestRenderPass test quad vertices");

    alloc = m_QuadIBuffer.Initialize(initContext, iList, _countof(iList), sizeof(uint16_t));
    alloc->SetName(L"GfxTestRenderPass test quad indices");
}

void GfxTestRenderPass::Render(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    context.GetCommandList().Dev()->SetName(L"GfxTestRenderPass::Render");

    context.ClearRenderTargetView(context.GetGfxManager().GetSwapChain().GetCurrentBackBuffer(), XMFLOAT4{ 0.0f, 0.2f, 0.4f, 1.0f });

    GfxShaderManager& shaderManager = context.GetShaderManager();

    GfxPipelineStateObject& pso = context.GetPSO();
    pso.SetDepthEnable(false);
    pso.SetDepthClipEnable(false);
    pso.SetStencilEnable(false);
    pso.SetVertexShader(shaderManager.GetShader(ShaderPermutation::VS_TestTriangle));
    pso.SetPixelShader(shaderManager.GetShader(ShaderPermutation::PS_TestTriangle));
    pso.SetVertexInputLayout(GfxDefaultVertexFormats::Position3f_TexCoord2f_Color4ub);

    context.SetVertexBuffer(m_QuadVBuffer);
    context.SetIndexBuffer(m_QuadIBuffer);
    context.SetRenderTarget(0, context.GetGfxManager().GetSwapChain().GetCurrentBackBuffer());

    context.DrawIndexedInstanced(1, 0, 0, 0);
}
