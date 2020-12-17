
#include <graphic/pch.h>

#include <system/imguimanager.h>

#include <tmp/shaders/autogen/cpp/ShaderInputs/NBodyGravityCSConsts.h>
#include <tmp/shaders/autogen/cpp/ShaderInputs/NBodyGravityVSPSConsts.h>
#include <tmp/shaders/autogen/cpp/ShaderInputs/BodyGravityPosVelo.h>
#include <tmp/shaders/autogen/cpp/ShaderPermutations/NBodyGravity.h>
#include <tmp/shaders/autogen/cpp/ShaderPermutations/NBodyGravityFill.h>

namespace BodyGravityParticles
{
    static const uint32_t C_MAX_PARTICLES = 1000;
    
    static bool gs_UpdateIMGUI = false;
    static uint32_t gs_ParticleCount = 500;

    static GfxTexture gs_ParticlesUAV;
}

class GfxBodyGravityParticlesUpdate : public GfxRendererBase
{
    void Initialize() override;
    void PopulateCommandList(GfxContext& context) override;
    bool ShouldPopulateCommandList(GfxContext&) const override { return BodyGravityParticles::gs_UpdateIMGUI; }

    void UpdateIMGUI();

    float m_ParticleSpread = 50.0f;
    float m_ParticlesDamping = 1.0f;
    float m_ParticleMass = 66.0f;

    std::size_t m_RootSigHash = 0;
    bool m_InitUAVBuffer = true;
};

void GfxBodyGravityParticlesUpdate::Initialize()
{
    bbeProfileFunction();

    g_IMGUIManager.RegisterTopMenu("Graphic", GetName(), &BodyGravityParticles::gs_UpdateIMGUI);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUI(); });

    // Perfomance TIP: Order from most frequent to least frequent.
    CD3DX12_DESCRIPTOR_RANGE1 ranges[2]{};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, AutoGenerated::NBodyGravityVSPSConsts::ConstantBufferRegister, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, AutoGenerated::NBodyGravityCSConsts::NbUAVs, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    m_RootSigHash = g_GfxRootSignatureManager.GetOrCreateRootSig(ranges, D3D12_ROOT_SIGNATURE_FLAG_NONE, "GfxBodyGravityParticlesUpdate_RootSignature");

    BodyGravityParticles::gs_ParticlesUAV.InitializeStructuredBuffer<AutoGenerated::BodyGravityPosVelo>(
        CD3DX12_RESOURCE_DESC1::Buffer(BodyGravityParticles::C_MAX_PARTICLES * sizeof(AutoGenerated::BodyGravityPosVelo), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        BodyGravityParticles::C_MAX_PARTICLES,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    BodyGravityParticles::gs_ParticlesUAV.SetDebugName("BodyGravityParticles_Buffer");
}

void GfxBodyGravityParticlesUpdate::PopulateCommandList(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    assert(m_RootSigHash);
    context.SetRootSignature(m_RootSigHash);

    AutoGenerated::NBodyGravityCSConsts consts;
    consts.m_DeltaTime = (float)g_System.GetFrameTimeMs() / 1000; // seconds
    consts.m_MaxParticles = BodyGravityParticles::gs_ParticleCount;
    consts.m_ParticlesDamping = m_ParticlesDamping;
    consts.m_TileSize = std::max(1U, BodyGravityParticles::gs_ParticleCount / AutoGenerated::NBodyGravityCSConsts::BlockSize);
    consts.m_ParticleMass = m_ParticleMass;
    consts.m_ParticleSpread = m_ParticleSpread;
    consts.m_ParticleCenterSpread = m_ParticleSpread * 0.5f;
    context.StageCBV(consts);

    context.StageUAV(BodyGravityParticles::gs_ParticlesUAV, 1, AutoGenerated::NBodyGravityCSConsts::UAVOffsets::PosVelo);

    if (m_InitUAVBuffer)
    {
        context.SetShader(g_GfxShaderManager.GetShader(Shaders::CS_NBodyGravityFill{}));
        context.Dispatch(bbeGetCSDispatchCount(BodyGravityParticles::C_MAX_PARTICLES, 64), 1, 1);
        m_InitUAVBuffer = false;
    }

    context.UAVBarrier(BodyGravityParticles::gs_ParticlesUAV);

    context.SetShader(g_GfxShaderManager.GetShader(Shaders::CS_NBodyGravity{}));
    context.Dispatch(consts.m_TileSize, 1, 1);
}

void GfxBodyGravityParticlesUpdate::UpdateIMGUI()
{
    if (!BodyGravityParticles::gs_UpdateIMGUI)
        return;

    ImGui::SliderFloat("Particles Damping", &m_ParticlesDamping, 0.1f, 2.0f);
    ImGui::SliderFloat("Particle Mass", &m_ParticleMass, 1.0f, 100.0f);

    bool reInitParticlesUAVBuffer = false;
    reInitParticlesUAVBuffer |= ImGui::SliderInt("Particle Count", &(int)BodyGravityParticles::gs_ParticleCount, 10, BodyGravityParticles::C_MAX_PARTICLES);
    reInitParticlesUAVBuffer |= ImGui::SliderFloat("Particle Spread", &m_ParticleSpread, 1.0f, 100.0f);

    if (reInitParticlesUAVBuffer)
    {
        g_GfxManager.AddGraphicCommand([this] { m_InitUAVBuffer = true; });
    }
}

class GfxBodyGravityParticlesRender : public GfxRendererBase
{
public:
    void Initialize() override;
    bool ShouldPopulateCommandList(GfxContext&) const override { return BodyGravityParticles::gs_UpdateIMGUI; }
    void PopulateCommandList(GfxContext& context) override;

    float m_ParticleRadius = 1.0f;

    std::size_t m_RootSigHash = 0;
};

void GfxBodyGravityParticlesRender::Initialize()
{
    bbeProfileFunction();

    // Perfomance TIP: Order from most frequent to least frequent.
    CD3DX12_DESCRIPTOR_RANGE1 ranges[2]{};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, AutoGenerated::NBodyGravityVSPSConsts::ConstantBufferRegister, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, AutoGenerated::NBodyGravityVSPSConsts::NbSRVs, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

    m_RootSigHash = g_GfxRootSignatureManager.GetOrCreateRootSig(ranges, D3D12_ROOT_SIGNATURE_FLAG_NONE, "GfxBodyGravityParticlesRender_RootSignature");
}

void GfxBodyGravityParticlesRender::PopulateCommandList(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    assert(m_RootSigHash);
    context.SetRootSignature(m_RootSigHash);

    context.SetRenderTarget(g_GfxManager.GetSwapChain().GetCurrentBackBuffer());

    // No vertex formats... we'll generate the positions of each particle quad directly in the VS via Vertex/Instance IDs
    context.SetVertexFormat(GfxDefaultVertexFormats::Null);

    context.SetTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Describe the blend and depth states.
    D3D12_RENDER_TARGET_BLEND_DESC blendDesc = GfxCommonStates::BlendAlphaAdditive;
    blendDesc.SrcBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    context.SetBlendStates(0, blendDesc);

    // Describe the blend and depth states.
    context.SetDepthStencilStates(GfxCommonStates::DepthNone);

    View& view = g_GfxManager.GetMainView();

    AutoGenerated::NBodyGravityVSPSConsts consts;
    consts.m_ViewProjMatrix = view.m_GfxVP;
    consts.m_InvViewProjMatrix = view.m_GfxInvVP;
    consts.m_ParticleRadius = m_ParticleRadius;
    context.StageCBV(consts);

    // The buffer was in UA state in earlier Updater renderer
    context.BeginTrackResourceState(BodyGravityParticles::gs_ParticlesUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    context.StageSRV(BodyGravityParticles::gs_ParticlesUAV, 1, AutoGenerated::NBodyGravityVSPSConsts::SRVOffsets::PosVelo);

    context.SetShader(g_GfxShaderManager.GetShader(Shaders::VS_NBodyGravity{}));
    context.SetShader(g_GfxShaderManager.GetShader(Shaders::PS_NBodyGravity{}));

    context.DrawInstanced(4, BodyGravityParticles::gs_ParticleCount);

    // transition back to UA state for updater renderer next frame
    context.TransitionResource(BodyGravityParticles::gs_ParticlesUAV, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);
}

REGISTER_RENDERER(GfxBodyGravityParticlesUpdate);
REGISTER_RENDERER(GfxBodyGravityParticlesRender);
