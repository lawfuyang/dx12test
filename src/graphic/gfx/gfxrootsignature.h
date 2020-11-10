#pragma once

class GfxRootSignature
{
public:
    static const uint32_t MaxRootParams = 8;

    ID3D12RootSignature* Dev() const { return m_RootSignature.Get(); }
    std::size_t GetHash() const { return m_Hash; }

    template <bool AllowInputAssembler, uint32_t NbRanges>
    void Compile(CD3DX12_DESCRIPTOR_RANGE1 (&ranges)[NbRanges], const char* rootSigName)
    {
        // Perfomance TIP: Order from most frequent to least frequent.
        CD3DX12_ROOT_PARAMETER1 rootParams[NbRanges];
        for (uint32_t i = 0; i < NbRanges; ++i)
        {
            rootParams[i].InitAsDescriptorTable(1, &ranges[i]);
        }

        CompileInternal(rootParams, NbRanges, AllowInputAssembler ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT : D3D12_ROOT_SIGNATURE_FLAG_NONE, rootSigName);
    }

private:
    void CompileInternal(CD3DX12_ROOT_PARAMETER1* rootParams, uint32_t numRootParams, D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags, const char* rootSigName);

    InplaceArray<CD3DX12_ROOT_PARAMETER1, MaxRootParams> m_RootParams;
    ComPtr<ID3D12RootSignature> m_RootSignature;
    std::size_t m_Hash = 0;

    friend class GfxContext;
};
