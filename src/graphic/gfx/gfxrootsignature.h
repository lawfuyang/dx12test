#pragma once

class GfxRootSignature
{
public:
    static const uint32_t MaxRootParams = 8;

    ID3D12RootSignature* Dev() const { return m_RootSignature.Get(); }
    std::size_t GetHash() const { return m_Hash; }

private:
    void Compile(CD3DX12_ROOT_PARAMETER1* rootParams, uint32_t numRootParams, D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags, const char* rootSigName);

    InplaceArray<CD3DX12_ROOT_PARAMETER1, MaxRootParams> m_RootParams;
    ComPtr<ID3D12RootSignature> m_RootSignature;
    std::size_t m_Hash = 0;

    friend class GfxContext;
    friend class GfxRootSignatureManager;
};

class GfxRootSignatureManager
{
    DeclareSingletonFunctions(GfxRootSignatureManager);
public:
    GfxRootSignature* GetOrCreateRootSig(CD3DX12_DESCRIPTOR_RANGE1* ranges, uint32_t nbRanges, D3D12_ROOT_SIGNATURE_FLAGS flags, const char* rootSigName);

private:
    std::unordered_map<std::size_t, GfxRootSignature> m_CachedRootSigs;
    std::mutex m_CachedRootSigsLock;
};
#define g_GfxRootSignatureManager GfxRootSignatureManager::GetInstance()
