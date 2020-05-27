#include "graphic/gfx/gfxrootsignature.h"

#include "graphic/gfx/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfx/gfxdevice.h"

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

void GfxRootSignature::AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t numDescriptors, uint32_t baseShaderRegister)
{
    CD3DX12_DESCRIPTOR_RANGE1 newRange;
    newRange.Init(type, numDescriptors, baseShaderRegister);
    newRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
    m_DescRanges.push_back(newRange);

    boost::hash_combine(m_Hash, newRange.RangeType);
    boost::hash_combine(m_Hash, newRange.NumDescriptors);
    boost::hash_combine(m_Hash, newRange.BaseShaderRegister);
}

void GfxRootSignature::Compile(const std::string& rootSigName)
{
    bbeProfileFunction();

    assert(m_RootSignature.Get() == nullptr);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    const D3D_ROOT_SIGNATURE_VERSION highestRootSigVer = gfxDevice.GetHighSupportedRootSignature();

    assert(highestRootSigVer == D3D_ROOT_SIGNATURE_VERSION_1_1);

    std::vector<CD3DX12_ROOT_PARAMETER1> rootParams;
    rootParams.resize(m_DescRanges.size());

    for (uint32_t i = 0; i < rootParams.size(); ++i)
    {
        rootParams[i].InitAsDescriptorTable(1, &m_DescRanges[i], D3D12_SHADER_VISIBILITY_ALL);
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSignatureDesc.Desc_1_1.NumParameters = rootParams.size();
    rootSignatureDesc.Desc_1_1.pParameters = rootParams.data();
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = _countof(gs_StaticSamplers);
    rootSignatureDesc.Desc_1_1.pStaticSamplers = gs_StaticSamplers;
    rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // change to D3D12_ROOT_SIGNATURE_FLAG_NONE for compute shaders etc

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DX12_CALL(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, highestRootSigVer, &signature, &error));
    DX12_CALL(gfxDevice.Dev()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    boost::hash_combine(m_Hash, rootSignatureDesc.Version);
    boost::hash_combine(m_Hash, rootSignatureDesc.Desc_1_1.Flags);

    m_RootSignature->SetName(MakeWStrFromStr(rootSigName).c_str());
}

void GfxRootSignatureManager::Initialize()
{
    bbeProfileFunction();

    assert(DefaultRootSignatures::DefaultGraphicsRootSignature.m_Hash == 0);
    DefaultRootSignatures::DefaultGraphicsRootSignature.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // FrameParams CBs
    DefaultRootSignatures::DefaultGraphicsRootSignature.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // Render Passes CBs
    DefaultRootSignatures::DefaultGraphicsRootSignature.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // 1 SRV
    DefaultRootSignatures::DefaultGraphicsRootSignature.Compile("DefaultGraphicsRootSignature");
}
