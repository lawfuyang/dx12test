#include "graphic/gfxrootsignature.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

static constexpr D3D12_STATIC_SAMPLER_DESC CreateStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addressMode, uint32_t shaderRegister)
{
    const uint32_t StaticSamplerSpaceIndex = 99; //  just shove it in space 99, so that it doesnt over lap with other non-static sampler registers

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter           = filter;
    sampler.AddressU         = addressMode;
    sampler.AddressV         = addressMode;
    sampler.AddressW         = addressMode;
    sampler.MipLODBias       = 0;
    sampler.MaxAnisotropy    = filter == D3D12_FILTER_ANISOTROPIC ? 16 : 0;
    sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.ComparisonFunc   = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.MinLOD           = 0.0f;
    sampler.MaxLOD           = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister   = shaderRegister;
    sampler.RegisterSpace    = StaticSamplerSpaceIndex;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    return sampler;
}
static constexpr D3D12_STATIC_SAMPLER_DESC gs_PointClampSamplerDesc       = CreateStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0);
static constexpr D3D12_STATIC_SAMPLER_DESC gs_PointWrapSamplerDesc        = CreateStaticSampler(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 1);
static constexpr D3D12_STATIC_SAMPLER_DESC gs_TrilinearClampSamplerDesc   = CreateStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 2);
static constexpr D3D12_STATIC_SAMPLER_DESC gs_TrilinearWrapSamplerDesc    = CreateStaticSampler(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 3);
static constexpr D3D12_STATIC_SAMPLER_DESC gs_AnisotropicClampSamplerDesc = CreateStaticSampler(D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 4);
static constexpr D3D12_STATIC_SAMPLER_DESC gs_AnisotropicWrapSamplerDesc  = CreateStaticSampler(D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 5);

static constexpr D3D12_STATIC_SAMPLER_DESC gs_StaticSamplers[] =
{
    gs_PointClampSamplerDesc,
    gs_PointWrapSamplerDesc,
    gs_TrilinearClampSamplerDesc,
    gs_TrilinearWrapSamplerDesc,
    gs_AnisotropicClampSamplerDesc,
    gs_AnisotropicWrapSamplerDesc,
};

void GfxRootSignature::AddRootParam(D3D12_DESCRIPTOR_RANGE_TYPE type)
{
    D3D12_DESCRIPTOR_RANGE1 newRange;
    newRange.RangeType = type;
    newRange.NumDescriptors = 1;
    newRange.BaseShaderRegister = m_ShaderRegisters[type]++;
    newRange.RegisterSpace = ms_RootSigID; // Use the root sig ID as register space
    newRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
    newRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    boost::hash_combine(m_Hash, newRange.RangeType);
    boost::hash_combine(m_Hash, newRange.NumDescriptors);
    boost::hash_combine(m_Hash, newRange.BaseShaderRegister);
    boost::hash_combine(m_Hash, newRange.RegisterSpace);
    boost::hash_combine(m_Hash, newRange.Flags);
    boost::hash_combine(m_Hash, newRange.OffsetInDescriptorsFromTableStart);

    CD3DX12_ROOT_PARAMETER1 rootParameter;
    rootParameter.InitAsDescriptorTable(1, &newRange, D3D12_SHADER_VISIBILITY_ALL);

    m_RootParams.push_back(rootParameter);
}

void GfxRootSignature::Compile()
{
    bbeProfileFunction();

    assert(m_RootSignature.Get() == nullptr);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    const D3D_ROOT_SIGNATURE_VERSION highestRootSigVer = gfxDevice.GetHighSupportedRootSignature();

    assert(highestRootSigVer == D3D_ROOT_SIGNATURE_VERSION_1_1);

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSignatureDesc.Desc_1_1.NumParameters = m_RootParams.size();
    rootSignatureDesc.Desc_1_1.pParameters = m_RootParams.data();
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = _countof(gs_StaticSamplers);
    rootSignatureDesc.Desc_1_1.pStaticSamplers = gs_StaticSamplers;
    rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // change to D3D12_ROOT_SIGNATURE_FLAG_NONE for compute shaders etc

    boost::hash_combine(m_Hash, rootSignatureDesc.Version);
    boost::hash_combine(m_Hash, rootSignatureDesc.Desc_1_1.Flags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DX12_CALL(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, highestRootSigVer, &signature, &error));
    DX12_CALL(gfxDevice.Dev()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    // clear all root params. we no longer need them.
    m_RootParams.resize(0);

    ++ms_RootSigID;
}

void GfxRootSignatureManager::Initialize()
{
    bbeProfileFunction();

    assert(DefaultRootSignatures::DefaultGraphicsRootSignature.m_Hash == 0);
    DefaultRootSignatures::DefaultGraphicsRootSignature.AddRootParam(D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
    DefaultRootSignatures::DefaultGraphicsRootSignature.Compile();
}
