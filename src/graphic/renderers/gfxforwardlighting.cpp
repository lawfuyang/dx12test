#include <graphic/renderers/gfxforwardlighting.h>

#include <system/imguimanager.h>

#include <graphic/visual.h>
#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxcontext.h>
#include <graphic/gfx/gfxswapchain.h>
#include <graphic/gfx/gfxdefaultassets.h>
#include <graphic/gfx/gfxlightsmanager.h>

#include <application/scene.h>

#include <tmp/shaders/autogen/cpp/PerFrameConsts.h>
#include <tmp/shaders/autogen/cpp/PerInstanceConsts.h>
#include <tmp/shaders/autogen/cpp/VS_ForwardLighting.h>
#include <tmp/shaders/autogen/cpp/PS_ForwardLighting.h>

static bool gs_ShowForwardLightingIMGUIWindow = false;
GfxTexture gs_SceneDepthBuffer;

void GfxForwardLightingPass::Initialize()
{
    bbeProfileFunction();

    g_IMGUIManager.RegisterTopMenu("Graphic", GetName(), &gs_ShowForwardLightingIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUI(); });

    // Keep ranges static so GfxContext can parse through them
    // Perfomance TIP: Order from most frequent to least frequent.
    static CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // PerInstanceConsts @ c0
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // PerFrameConsts @ c1
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // Per Visual textures @ t0-t2

    m_RootSignature = g_GfxRootSignatureManager.GetOrCreateRootSig(ranges, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "GfxForwardLightingPass_RootSignature");

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

    gs_SceneDepthBuffer.Initialize(depthBufferInitParams);
}

void GfxForwardLightingPass::ShutDown()
{
    gs_SceneDepthBuffer.Release();
}

void GfxForwardLightingPass::PopulateCommandList(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    assert(m_RootSignature);
    context.SetRootSignature(*m_RootSignature);

    View& view = g_GfxManager.GetMainView();

    AutoGenerated::PerFrameConsts frameConsts;
    frameConsts.m_ViewProjMatrix = view.m_GfxVP;
    frameConsts.m_CameraPosition = bbeVector4{ view.m_Eye };
    frameConsts.m_SceneLightDir = bbeVector4{ g_GfxLightsManager.GetDirectionalLight().m_Direction };
    frameConsts.m_SceneLightIntensity = bbeVector4{ g_GfxLightsManager.GetDirectionalLight().m_Intensity };
    frameConsts.m_ConstPBRMetallic = m_ConstPBRMetallic;
    frameConsts.m_ConstPBRRoughness = m_ConstPBRRoughness;
    UploadConstantBufferToGPU(context, frameConsts, 1);

    context.SetRenderTarget(0, g_GfxManager.GetSwapChain().GetCurrentBackBuffer());
    context.SetDepthStencil(gs_SceneDepthBuffer);

    GfxPipelineStateObject& pso = context.GetPSO();

    Shaders::PS_ForwardLightingPermutations psPerms;
    psPerms.USE_PBR_CONSTS = m_UsePBRConsts;
    pso.SetPixelShader(g_GfxShaderManager.GetShader(psPerms));

    for (Visual* visual : g_Scene.m_AllVisuals)
    {
        assert(visual->IsValid());

        GfxVertexFormat& vFormat = visual->m_Mesh->GetVertexFormat();
        pso.SetVertexFormat(vFormat);

        Shaders::VS_ForwardLightingPermutations vsPerms;
        vsPerms.VERTEX_FORMAT_Position2f_TexCoord2f_Color4ub = vFormat == GfxDefaultVertexFormats::Position2f_TexCoord2f_Color4ub;
        vsPerms.VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f = vFormat == GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f;
        vsPerms.VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f = vFormat == GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f_Tangent3f;
        pso.SetVertexShader(g_GfxShaderManager.GetShader(vsPerms));

        context.SetVertexBuffer(visual->m_Mesh->GetVertexBuffer());
        context.SetIndexBuffer(visual->m_Mesh->GetIndexBuffer());

        context.StageSRV(*visual->m_DiffuseTexture, 2, 0);
        context.StageSRV(*visual->m_NormalTexture, 2, 1);
        context.StageSRV(*visual->m_ORMTexture, 2, 2);

        bbeMatrix worldMat = bbeMatrix::CreateTranslation(visual->m_WorldPosition) * bbeMatrix::CreateFromQuaternion(visual->m_Rotation) * bbeMatrix::CreateScale(visual->m_Scale);
        worldMat = worldMat.Transpose();

        AutoGenerated::PerInstanceConsts instanceConsts;
        instanceConsts.m_WorldMatrix = worldMat;
        UploadConstantBufferToGPU(context, instanceConsts, 0);

        context.DrawIndexedInstanced(visual->m_Mesh->GetIndexBuffer().GetNumIndices(), 1, 0, 0, 0);
    }
}

void GfxForwardLightingPass::UpdateIMGUI()
{
    if (!gs_ShowForwardLightingIMGUIWindow)
        return;

    ScopedIMGUIWindow window{ "GfxForwardLightingPass" };

    if (ImGui::CollapsingHeader("PBR Consts", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Over-ride PBR consts", &m_UsePBRConsts);
        ImGui::SliderFloat("PBR Roughness", &m_ConstPBRRoughness, 0.0f, 1.0f);
        ImGui::SliderFloat("PBR Metallic", &m_ConstPBRMetallic, 0.0f, 1.0f);
    }
}

static GfxForwardLightingPass gs_GfxForwardLightingPass;
GfxRendererBase* g_GfxForwardLightingPass = &gs_GfxForwardLightingPass;
