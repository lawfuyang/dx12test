#include <graphic/renderers/gfxzprepassrenderer.h>

#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxview.h>
#include <graphic/gfx/gfxdefaultassets.h>

#include <tmp/shaders/autogen/cpp/PerFrameConsts.h>
#include <tmp/shaders/autogen/cpp/VS_UberShader.h>
#include <tmp/shaders/autogen/cpp/PS_UberShader.h>

void GfxZPrePassRenderer::Initialize()
{
    bbeProfileFunction();
    m_Name = "GfxTestRenderPass";

    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxZPrePassRenderer::Initialize");

    m_RenderPassCB.Initialize<AutoGenerated::PerFrameConsts>(true);

    // Perfomance TIP: Order from most frequent to least frequent.
    CD3DX12_ROOT_PARAMETER1 rootParams[1];
    rootParams[0].InitAsConstantBufferView(0); // 1 CBV in c0

    m_RootSignature.Compile(rootParams, _countof(rootParams), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "GfxZPrePassRenderer_RootSignature");

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());
}

void GfxZPrePassRenderer::ShutDown()
{
    m_RenderPassCB.Release();
}

void GfxZPrePassRenderer::PopulateCommandList()
{
    bbeProfileFunction();

    GfxContext& context = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxZPrePassRenderer");
    m_Context = &context;
    context.SetRootSignature(m_RootSignature);

    bbeProfileGPUFunction(context);

    AutoGenerated::PerFrameConsts consts;
    consts.m_ViewProjMatrix = g_GfxView.GetViewProjMatrix();
    m_RenderPassCB.Update(&consts);

    context.SetRootCBV(m_RenderPassCB, 0);

    GfxPipelineStateObject& pso = context.GetPSO();

    Shaders::VS_UberShaderPermutations vsPerms;
    vsPerms.VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f = true;

    const GfxShader& vShader = g_GfxShaderManager.GetShader(vsPerms);
    pso.SetVertexShader(vShader);

    context.SetDepthStencil(g_GfxManager.GetSceneDepthBuffer());

    GfxDefaultAssets::DrawSquidRoom(context, false);
}
