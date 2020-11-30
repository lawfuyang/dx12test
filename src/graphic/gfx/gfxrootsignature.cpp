#include <graphic/gfx/gfxrootsignature.h>
#include <graphic/pch.h>

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

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    const D3D_ROOT_SIGNATURE_VERSION highestRootSigVer = gfxDevice.GetHighSupportedRootSignature();

    assert(highestRootSigVer == D3D_ROOT_SIGNATURE_VERSION_1_1);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(numRootParams, rootParams, _countof(gs_StaticSamplers), gs_StaticSamplers, rootSigFlags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DX12_CALL(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, highestRootSigVer, &signature, &error));
    DX12_CALL(gfxDevice.Dev()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));

    // parse root sig to prepare container of staged resources' descriptors
    for (uint32_t i = 0; i < numRootParams; ++i)
    {
        StagedResourcesDescriptors& param = m_Params.emplace_back();

        // No descriptor staging and copying needed for root params & constants
        if (rootParams[i].ParameterType != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            continue;

        const D3D12_ROOT_DESCRIPTOR_TABLE1& rootTable = rootParams[i].DescriptorTable;
        for (uint32_t rangeIdx = 0; rangeIdx < rootTable.NumDescriptorRanges; ++rangeIdx)
        {
            const D3D12_DESCRIPTOR_RANGE1& range = rootTable.pDescriptorRanges[rangeIdx];
            for (uint32_t i = 0; i < range.NumDescriptors; ++i)
            {
                param.m_Types.push_back(range.RangeType);
                param.m_Descriptors.emplace_back(CD3DX12_DEFAULT{});
            }
        }
    }

    m_RootSignature->SetName(StringUtils::Utf8ToWide(rootSigName));
}

GfxRootSignature* GfxRootSignatureManager::GetOrCreateRootSig(CD3DX12_DESCRIPTOR_RANGE1* ranges, uint32_t nbRanges, D3D12_ROOT_SIGNATURE_FLAGS flags, const char* rootSigName)
{
    assert(nbRanges < GfxRootSignature::MaxRootParams);
    CD3DX12_ROOT_PARAMETER1 rootParams[GfxRootSignature::MaxRootParams];

    for (uint32_t i = 0; i < nbRanges; ++i)
        rootParams[i].InitAsDescriptorTable(1, &ranges[i]);

    std::size_t hash = 0;
    boost::hash_combine(hash, flags);
    for (uint32_t i = 0; i < nbRanges; ++i)
    {
        boost::hash_combine(hash, rootParams[i].ParameterType);
        boost::hash_combine(hash, rootParams[i].ShaderVisibility);
    }

    bbeAutoLock(m_CachedRootSigsLock);
    GfxRootSignature& rootSig = m_CachedRootSigs[hash];

    // It's cached. Return it
    if (rootSig.m_RootSignature.Get() != nullptr)
    {
        assert(rootSig.m_Hash != 0);
        assert(!rootSig.m_Params.empty());
        return &rootSig;
    }

    rootSig.m_Hash = hash;
    rootSig.Compile(rootParams, nbRanges, flags, rootSigName);
    return &rootSig;
}
