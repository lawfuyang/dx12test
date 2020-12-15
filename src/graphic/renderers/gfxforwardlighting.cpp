#include <graphic/pch.h>

#include <system/imguimanager.h>

#include <application/scene.h>

#include <tmp/shaders/autogen/cpp/ShaderInputs/PerFrameConsts.h>
#include <tmp/shaders/autogen/cpp/ShaderInputs/PerInstanceConsts.h>
#include <tmp/shaders/autogen/cpp/ShaderPermutations/ForwardLighting.h>

static bool gs_ShowForwardLightingIMGUIWindow = false;
GfxTexture g_SceneDepthBuffer;

class GfxForwardLightingPass : public GfxRendererBase
{
    void Initialize() override;
    bool ShouldPopulateCommandList(GfxContext&) const override;
    void PopulateCommandList(GfxContext& context) override;
    const char* GetName() const override { return "GfxForwardLightingPass"; }

    void UpdateIMGUI();

    GfxRootSignature* m_RootSignature                  = nullptr;
    bool              m_ShowForwardLightingIMGUIWindow = false;
    bool              m_UsePBRConsts                   = false;
    float             m_ConstPBRRoughness              = 0.75f;
    float             m_ConstPBRMetallic               = 0.1f;
};

void GfxForwardLightingPass::Initialize()
{
    bbeProfileFunction();

    g_IMGUIManager.RegisterTopMenu("Graphic", GetName(), &m_ShowForwardLightingIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUI(); });

    // Perfomance TIP: Order from most frequent to least frequent.
    CD3DX12_DESCRIPTOR_RANGE1 ranges[3]{};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, AutoGenerated::PerInstanceConsts::ConstantBufferRegister, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // PerInstanceConsts CBV @ c0
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, AutoGenerated::PerFrameConsts::ConstantBufferRegister, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // PerFrameConsts CBV @ c1
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, AutoGenerated::PerInstanceConsts::NbSRVs, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // PerInstanceConsts SRVs @ t0-t2

    m_RootSignature = g_GfxRootSignatureManager.GetOrCreateRootSig(ranges, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "GfxForwardLightingPass_RootSignature");

    CD3DX12_RESOURCE_DESC1 desc{ CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT_D32_FLOAT, g_CommandLineOptions.m_WindowWidth, g_CommandLineOptions.m_WindowHeight) };
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    g_SceneDepthBuffer.InitializeTexture(desc, nullptr, { desc.Format, { 1.0f, 0 } }, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    g_SceneDepthBuffer.SetDebugName("SceneDepthBuffer");
}

bool GfxForwardLightingPass::ShouldPopulateCommandList(GfxContext&) const
{
    return !g_Scene.m_AllVisuals.empty();
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
    context.StageCBV(frameConsts);

    context.SetRenderTarget(g_GfxManager.GetSwapChain().GetCurrentBackBuffer());
    context.SetDepthStencil(g_SceneDepthBuffer);

    Shaders::PS_ForwardLighting psPerms;
    psPerms.USE_PBR_CONSTS = m_UsePBRConsts;
    context.SetShader(g_GfxShaderManager.GetShader(psPerms));

    for (Visual* visual : g_Scene.m_AllVisuals)
    {
        assert(visual->IsValid());

        GfxVertexFormat& vFormat = *visual->m_Mesh->m_VertexFormat;
        context.SetVertexFormat(vFormat);

        Shaders::VS_ForwardLighting vsPerms;
        vsPerms.VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f = vFormat == GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f;
        vsPerms.VERTEX_FORMAT_Position3f_Normal3f_Texcoord2f_Tangent3f = vFormat == GfxDefaultVertexFormats::Position3f_Normal3f_Texcoord2f_Tangent3f;
        context.SetShader(g_GfxShaderManager.GetShader(vsPerms));

        using SRVOffsets = AutoGenerated::PerInstanceConsts::SRVOffsets;
        context.StageSRV(*visual->m_DiffuseTexture, 2, SRVOffsets::DiffuseTexture);
        context.StageSRV(*visual->m_NormalTexture, 2, SRVOffsets::NormalTexture);
        context.StageSRV(*visual->m_ORMTexture, 2, SRVOffsets::ORMTexture);

        bbeMatrix worldMat = bbeMatrix::CreateTranslation(visual->m_WorldPosition) * bbeMatrix::CreateFromQuaternion(visual->m_Rotation) * bbeMatrix::CreateScale(visual->m_Scale);
        worldMat = worldMat.Transpose();

        AutoGenerated::PerInstanceConsts instanceConsts;
        instanceConsts.m_WorldMatrix = worldMat;
        context.StageCBV(instanceConsts);

        context.SetVertexBuffer(visual->m_Mesh->m_VertexBuffer);
        context.SetIndexBuffer(visual->m_Mesh->m_IndexBuffer);

        context.DrawIndexedInstanced(visual->m_Mesh->m_IndexBuffer.GetNumIndices(), 1, 0, 0, 0);
    }
}

void GfxForwardLightingPass::UpdateIMGUI()
{
    if (!m_ShowForwardLightingIMGUIWindow)
        return;

    ScopedIMGUIWindow window{ "GfxForwardLightingPass" };

    if (ImGui::CollapsingHeader("PBR Consts", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Over-ride PBR consts", &m_UsePBRConsts);
        ImGui::SliderFloat("PBR Roughness", &m_ConstPBRRoughness, 0.0f, 1.0f);
        ImGui::SliderFloat("PBR Metallic", &m_ConstPBRMetallic, 0.0f, 1.0f);
    }
}

REGISTER_RENDERER(GfxForwardLightingPass);
