#include <graphic/renderers/gfxbodygravityparticles.h>

#include <graphic/gfx/gfxrootsignature.h>

void GfxBodyGravityParticlesUpdate::Initialize()
{
    // Keep ranges static so GfxContext can parse through them
    // Perfomance TIP: Order from most frequent to least frequent.
    static CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // input particle PosVelo SRV
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);        // output particle PosVelo UAV

    m_RootSignature = g_GfxRootSignatureManager.GetOrCreateRootSig(ranges, D3D12_ROOT_SIGNATURE_FLAG_NONE, "GfxBodyGravityParticlesUpdate_RootSignature");
}

void GfxBodyGravityParticlesUpdate::PopulateCommandList(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    assert(m_RootSignature);
    context.SetRootSignature(*m_RootSignature);
}

static GfxBodyGravityParticlesUpdate gs_GfxBodyGravityParticlesUpdate;
GfxRendererBase* g_GfxBodyGravityParticlesUpdate = &gs_GfxBodyGravityParticlesUpdate;



void GfxBodyGravityParticlesRender::Initialize()
{
    // Keep ranges static so GfxContext can parse through them
    // Perfomance TIP: Order from most frequent to least frequent.
    static CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE); // input particle PosVelo SRV

    m_RootSignature = g_GfxRootSignatureManager.GetOrCreateRootSig(ranges, D3D12_ROOT_SIGNATURE_FLAG_NONE, "GfxBodyGravityParticlesUpdate_RootSignature");
}

void GfxBodyGravityParticlesRender::PopulateCommandList(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    assert(m_RootSignature);
    context.SetRootSignature(*m_RootSignature);
}

static GfxBodyGravityParticlesRender gs_GfxBodyGravityParticlesRender;
GfxRendererBase* g_GfxBodyGravityParticlesRender = &gs_GfxBodyGravityParticlesRender;
