#include "graphic/renderpasses/gfxtestrenderpass.h"

#include "graphic/gfxmanager.h"
#include "graphic/gfxcontext.h"
#include "graphic/gfxswapchain.h"
#include "graphic/gfxview.h"
#include "graphic/gfxdefaulttextures.h"

void GfxTestRenderPass::Initialize()
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxTestRenderPass::Initialize");

    bbeProfileGPUFunction(initContext);

    m_Name = "GfxTestRenderPass";

    struct Vertex
    {
        bbeVector3 m_Position = {};
        bbeVector2 m_TexCoord = {};
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

    GfxVertexBuffer::InitParams vBufferInitParams;
    vBufferInitParams.m_InitData = vList;
    vBufferInitParams.m_NumVertices = _countof(vList);
    vBufferInitParams.m_VertexSize = sizeof(Vertex);
    vBufferInitParams.m_ResourceName = "GfxTestRenderPass GfxVertexBuffer";

    GfxIndexBuffer::InitParams iBufferInitParams;
    iBufferInitParams.m_InitData = iList;
    iBufferInitParams.m_NumIndices = _countof(iList);
    iBufferInitParams.m_IndexSize = sizeof(uint16_t);
    iBufferInitParams.m_ResourceName = "GfxTestRenderPass GfxIndexBuffer";

    m_QuadVBuffer.Initialize(initContext, vBufferInitParams);
    m_QuadIBuffer.Initialize(initContext, iBufferInitParams);
    m_RenderPassCB.Initialize<AutoGenerated::TestShaderConsts>();
}

void GfxTestRenderPass::ShutDown()
{
    m_QuadVBuffer.Release();
    m_QuadIBuffer.Release();
    m_RenderPassCB.Release();
}

void GfxTestRenderPass::Render(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    AutoGenerated::TestShaderConsts consts;
    consts.m_ViewProjMatrix = g_GfxView.GetViewProjMatrix();
    m_RenderPassCB.Update(&consts);

    context.BindConstantBuffer(m_RenderPassCB);
    context.BindSRV(GfxDefaultTextures::Checkerboard);

    GfxPipelineStateObject& pso = context.GetPSO();

    CD3DX12_DEPTH_STENCIL_DESC1& depthDesc = pso.GetDepthStencilStates();
    depthDesc.DepthEnable = false;
    depthDesc.StencilEnable = false;

    CD3DX12_RASTERIZER_DESC& rasterDesc = pso.GetRasterizerStates();
    rasterDesc.DepthClipEnable = false;

    pso.SetVertexShader(g_GfxShaderManager.GetShader(ShaderPermutation::VS_TestTriangle));
    pso.SetPixelShader(g_GfxShaderManager.GetShader(ShaderPermutation::PS_TestTriangle));
    pso.SetVertexInputLayout(GfxDefaultVertexFormats::Position3f_TexCoord2f_Color4ub);

    context.SetVertexBuffer(m_QuadVBuffer);
    context.SetIndexBuffer(m_QuadIBuffer);
    context.SetRenderTarget(0, g_GfxManager.GetSwapChain().GetCurrentBackBuffer());

    context.ClearRenderTargetView(g_GfxManager.GetSwapChain().GetCurrentBackBuffer(), bbeVector4{ 0.0f, 0.2f, 0.4f, 1.0f });

    context.DrawIndexedInstanced(m_QuadIBuffer.GetSizeInBytes() / 2, 1, 0, 0, 0);
}
