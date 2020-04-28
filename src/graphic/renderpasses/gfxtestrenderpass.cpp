#include <graphic/renderpasses/gfxtestrenderpass.h>

#include <graphic/gfxmanager.h>
#include <graphic/gfxcontext.h>
#include <graphic/gfxswapchain.h>
#include <graphic/gfxview.h>
#include <graphic/gfxdefaultassets.h>

#include <tmp/shaders/TestShaderConsts.h>

void GfxTestRenderPass::Initialize()
{
    bbeProfileFunction();
    m_Name = "GfxTestRenderPass";

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxTestRenderPass::Initialize");

    GfxTexture::InitParams depthBufferInitParams;
    depthBufferInitParams.m_Format = DXGI_FORMAT_D32_FLOAT;
    depthBufferInitParams.m_Width = g_CommandLineOptions.m_WindowWidth;
    depthBufferInitParams.m_Height = g_CommandLineOptions.m_WindowHeight;
    depthBufferInitParams.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    depthBufferInitParams.m_InitData = nullptr;
    depthBufferInitParams.m_ResourceName = "GfxTestRenderPass Depth Buffer";
    depthBufferInitParams.m_InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    depthBufferInitParams.m_ViewType = GfxTexture::DSV;
    depthBufferInitParams.m_ClearValue.DepthStencil.Depth = 1.0f;
    depthBufferInitParams.m_ClearValue.DepthStencil.Stencil = 0;

    m_DepthBuffer.Initialize(initContext, depthBufferInitParams);
    m_RenderPassCB.Initialize<AutoGenerated::TestShaderConsts>(initContext);
}

void GfxTestRenderPass::ShutDown()
{
    m_RenderPassCB.Release();
    m_DepthBuffer.Release();
}

void GfxTestRenderPass::Render(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    AutoGenerated::TestShaderConsts consts;
    consts.m_ViewProjMatrix = g_GfxView.GetViewProjMatrix();
    m_RenderPassCB.Update(&consts);

    context.BindConstantBuffer(m_RenderPassCB);
    context.BindSRV(GfxDefaultAssets::Checkerboard);

    GfxPipelineStateObject& pso = context.GetPSO();

    CD3DX12_DEPTH_STENCIL_DESC1& depthDesc = pso.GetDepthStencilStates();
    depthDesc.DepthEnable = false;
    depthDesc.StencilEnable = false;

    CD3DX12_RASTERIZER_DESC& rasterDesc = pso.GetRasterizerStates();
    rasterDesc.DepthClipEnable = false;

    GfxMesh& mesh = GfxDefaultAssets::Occcity;

    pso.SetVertexShader(g_GfxShaderManager.GetShader(ShaderPermutation::VS_TestTriangle));
    pso.SetPixelShader(g_GfxShaderManager.GetShader(ShaderPermutation::PS_TestTriangle));
    pso.SetVertexFormat(mesh.GetVertexFormat());

    context.SetVertexBuffer(mesh.GetVertexBuffer());
    context.SetIndexBuffer(mesh.GetIndexBuffer());
    context.SetRenderTarget(0, g_GfxManager.GetSwapChain().GetCurrentBackBuffer());

    context.DrawIndexedInstanced(mesh.GetIndexBuffer().GetNumIndices(), 1, 0, 0, 0);
}
