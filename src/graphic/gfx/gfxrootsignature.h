#pragma once

static const uint32_t gs_MaxRootSigParams = 8;

class GfxRootSignature
{
public:
    static const uint32_t MaxRootParams = 8;

    ID3D12RootSignature* Dev() const { return m_RootSignature.Get(); }
    std::size_t GetHash() const { return m_Hash; }

private:
    void Compile(CD3DX12_ROOT_PARAMETER1* rootParams, uint32_t numRootParams, D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags, const char* rootSigName);

    struct StagedResourcesDescriptors
    {
        std::vector<D3D12_DESCRIPTOR_RANGE_TYPE> m_Types;
        std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_Descriptors;
    };
    using RootSigParams = InplaceArray<StagedResourcesDescriptors, gs_MaxRootSigParams>;
    RootSigParams m_Params;

    ComPtr<ID3D12RootSignature> m_RootSignature;
    std::size_t m_Hash = 0;

    friend class GfxContext;
    friend class GfxRootSignatureManager;
};

class GfxRootSignatureManager
{
    DeclareSingletonFunctions(GfxRootSignatureManager);
public:

    // helper to pass in ref to array of ranges
    template <uint32_t NbRanges>
    GfxRootSignature* GetOrCreateRootSig(CD3DX12_DESCRIPTOR_RANGE1(&ranges)[NbRanges], D3D12_ROOT_SIGNATURE_FLAGS flags, const char* rootSigName)
    {
        return GetOrCreateRootSig((CD3DX12_DESCRIPTOR_RANGE1*)&ranges, NbRanges, flags, rootSigName);
    }

    GfxRootSignature* GetOrCreateRootSig(CD3DX12_DESCRIPTOR_RANGE1* ranges, uint32_t nbRanges, D3D12_ROOT_SIGNATURE_FLAGS flags, const char* rootSigName);

private:
    std::unordered_map<std::size_t, GfxRootSignature> m_CachedRootSigs;
    std::shared_mutex m_CachedRootSigsRWLock;
};
#define g_GfxRootSignatureManager GfxRootSignatureManager::GetInstance()
