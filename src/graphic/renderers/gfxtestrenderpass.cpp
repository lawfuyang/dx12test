#include <graphic/renderers/gfxtestrenderpass.h>

#include <system/imguimanager.h>

#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxswapchain.h>
#include <graphic/gfx/gfxview.h>
#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/gfx/gfxlightsmanager.h>

#include <tmp/shaders/autogen/cpp/PerFrameConsts.h>
#include <tmp/shaders/autogen/cpp/VS_UberShader.h>
#include <tmp/shaders/autogen/cpp/PS_UberShader.h>

void GfxTestRenderPass::Initialize()
{
    bbeProfileFunction();
    m_Name = "GfxTestRenderPass";

    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxTestRenderPass::Initialize");

    m_RenderPassCB.Initialize<AutoGenerated::PerFrameConsts>(true);

    // Keep ranges static so GfxContext can parse through them
    static CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

    // Perfomance TIP: Order from most frequent to least frequent.
    CD3DX12_ROOT_PARAMETER1 rootParams[2];
    rootParams[0].InitAsConstantBufferView(0); // 1 CBV in c0
    rootParams[1].InitAsDescriptorTable(1, &ranges[0]); // 2 SRVs in t0-t1

    m_RootSignature.Compile(rootParams, _countof(rootParams), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "GfxTestRenderPass_RootSignature");

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());
}

void GfxTestRenderPass::ShutDown()
{
    m_RenderPassCB.Release();
}

void GfxTestRenderPass::PopulateCommandList()
{
    bbeProfileFunction();

    GfxContext& context = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxTestRenderPass");
    m_Context = &context;
    context.SetRootSignature(m_RootSignature);

    bbeProfileGPUFunction(context);

    AutoGenerated::PerFrameConsts consts;
    consts.m_ViewProjMatrix = g_GfxView.GetViewProjMatrix();
    consts.m_SceneLightDir = g_GfxLightsManager.GetDirectionalLight().m_Direction;
    consts.m_SceneLightIntensity = g_GfxLightsManager.GetDirectionalLight().m_Intensity;
    m_RenderPassCB.Update(&consts);

    context.SetRootCBV(m_RenderPassCB, 0);

    GfxPipelineStateObject& pso = context.GetPSO();

    Shaders::VS_UberShaderPermutations vPerms;
    vPerms.VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f = true;

    const GfxShader& vShader = g_GfxShaderManager.GetShader(vPerms);
    const GfxShader& pShader = g_GfxShaderManager.GetShader(Shaders::PS_UberShaderPermutations{});

    pso.SetVertexShader(vShader);
    pso.SetPixelShader(pShader);

    context.SetRenderTarget(0, g_GfxManager.GetSwapChain().GetCurrentBackBuffer());
    context.SetDepthStencil(g_GfxManager.GetSceneDepthBuffer());

    const uint32_t diffuseTexRootOffset = 0;
    const uint32_t normalTexRootOffset = 1;
    GfxDefaultAssets::DrawSquidRoom(context, true, 1, diffuseTexRootOffset, normalTexRootOffset);
}
