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
    sampler.BorderColor      = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
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

void GfxRootSignature::Compile(CD3DX12_ROOT_PARAMETER1* rootParams, uint32_t numRootParams, D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags, const char* rootSigName)
{
    assert(numRootParams < MaxRootParams);
    assert(m_RootSignature.Get() == nullptr);
    assert(m_RootParams.empty());

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    const D3D_ROOT_SIGNATURE_VERSION highestRootSigVer = gfxDevice.GetHighSupportedRootSignature();

    assert(highestRootSigVer == D3D_ROOT_SIGNATURE_VERSION_1_1);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(numRootParams, rootParams, _countof(gs_StaticSamplers), gs_StaticSamplers, rootSigFlags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DX12_CALL(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, highestRootSigVer, &signature, &error));
    DX12_CALL(gfxDevice.Dev()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    // copy root params to an internal array so GfxContext can parse through them
    m_RootParams.resize(numRootParams);
    memcpy(m_RootParams.data(), rootParams, sizeof(CD3DX12_ROOT_PARAMETER1) * numRootParams);

    m_RootSignature->SetName(MakeWStrFromStr(rootSigName).c_str());
    m_Hash = std::hash<std::string_view>{}(rootSigName);

    for (uint32_t i = 0; i < numRootParams; ++i)
    {
        boost::hash_combine(m_Hash, rootParams[i].ParameterType);
        boost::hash_combine(m_Hash, rootParams[i].ShaderVisibility);
    }
}
