#pragma once

class GfxRootSignature
{
public:
    static const uint32_t MaxRootParams = 8;

    ID3D12RootSignature* Dev() const { return m_RootSignature.Get(); }

    static void InitDefaultRootSignatures();

    std::size_t GetHash() const { return m_Hash; }

private:
    template <uint32_t NumRootParams>
    void Compile(const CD3DX12_ROOT_PARAMETER1 (&rootParams)[NumRootParams], D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags);

    InplaceArray<CD3DX12_ROOT_PARAMETER1, MaxRootParams> m_RootParams;
    ComPtr<ID3D12RootSignature> m_RootSignature;
    std::size_t m_Hash = 0;

    friend class GfxContext;
};

namespace GfxDefaultRootSignatures
{
    extern GfxRootSignature CBV1_SRV1_IA;
};
