#include <graphic/renderers/gfxforwardlighting.h>

#include <system/imguimanager.h>

#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxswapchain.h>
#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/gfx/gfxlightsmanager.h>

#include <tmp/shaders/autogen/cpp/PerFrameConsts.h>
#include <tmp/shaders/autogen/cpp/VS_ForwardLighting.h>
#include <tmp/shaders/autogen/cpp/PS_ForwardLighting.h>

GfxTexture gs_SceneDepthBuffer;

void GfxForwardLightingPass::Initialize()
{
    bbeProfileFunction();

    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxForwardLightingPass::Initialize");

    m_RenderPassCB.Initialize<AutoGenerated::PerFrameConsts>();

    // Keep ranges static so GfxContext can parse through them
    static CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

    // Perfomance TIP: Order from most frequent to least frequent.
    CD3DX12_ROOT_PARAMETER1 rootParams[2];
    rootParams[0].InitAsDescriptorTable(1, &ranges[0]); // 1 CBV in c0
    rootParams[1].InitAsDescriptorTable(1, &ranges[1]); // 2 SRVs in t0-t1

    m_RootSignature.Compile(rootParams, _countof(rootParams), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "GfxForwardLightingPass_RootSignature");

    GfxTexture::InitParams depthBufferInitParams;
    depthBufferInitParams.m_Format = DXGI_FORMAT_D32_FLOAT;
    depthBufferInitParams.m_Width = g_CommandLineOptions.m_WindowWidth;
    depthBufferInitParams.m_Height = g_CommandLineOptions.m_WindowHeight;
    depthBufferInitParams.m_Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    depthBufferInitParams.m_InitData = nullptr;
    depthBufferInitParams.m_ResourceName = "GfxManager Depth Buffer";
    depthBufferInitParams.m_InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    depthBufferInitParams.m_ViewType = GfxTexture::DSV;
    depthBufferInitParams.m_ClearValue.Format = depthBufferInitParams.m_Format;
    depthBufferInitParams.m_ClearValue.DepthStencil.Depth = 1.0f;
    depthBufferInitParams.m_ClearValue.DepthStencil.Stencil = 0;

    gs_SceneDepthBuffer.Initialize(initContext, depthBufferInitParams);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());
}

void GfxForwardLightingPass::ShutDown()
{
    m_RenderPassCB.Release();
    gs_SceneDepthBuffer.Release();
}

void GfxForwardLightingPass::PopulateCommandList()
{
    bbeProfileFunction();

    GfxContext& context = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxForwardLightingPass");
    m_Context = &context;
    context.SetRootSignature(m_RootSignature);

    bbeProfileGPUFunction(context);

    View& view = g_GfxManager.GetMainView();

    AutoGenerated::PerFrameConsts consts;
    consts.m_ViewProjMatrix = view.m_GfxVP;
    consts.m_CameraPosition = bbeVector4{ view.m_Eye };
    consts.m_SceneLightDir = bbeVector4{ g_GfxLightsManager.GetDirectionalLight().m_Direction };
    consts.m_SceneLightIntensity = bbeVector4{ g_GfxLightsManager.GetDirectionalLight().m_Intensity };
    m_RenderPassCB.Update(&consts);

    context.StageCBV(m_RenderPassCB, 0, 0);

    GfxPipelineStateObject& pso = context.GetPSO();

    Shaders::VS_ForwardLightingPermutations vPerms;
    vPerms.VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f = true;

    const GfxShader& vShader = g_GfxShaderManager.GetShader(vPerms);
    const GfxShader& pShader = g_GfxShaderManager.GetShader(Shaders::PS_ForwardLightingPermutations{});

    pso.SetVertexShader(vShader);
    pso.SetPixelShader(pShader);

    context.SetRenderTarget(0, g_GfxManager.GetSwapChain().GetCurrentBackBuffer());
    context.SetDepthStencil(gs_SceneDepthBuffer);

    const uint32_t diffuseTexRootOffset = 0;
    const uint32_t normalTexRootOffset = 1;
    GfxDefaultAssets::DrawSquidRoom(context, true, 1, diffuseTexRootOffset, normalTexRootOffset);
}