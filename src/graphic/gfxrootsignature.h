#pragma once

class GfxRootSignature
{
public:
    ID3D12RootSignature* Dev() const { return m_RootSignature.Get(); }

    void AddRootParam(D3D12_DESCRIPTOR_RANGE_TYPE);
    void Compile();

    uint32_t GetID() const { return ms_RootSigID; }

    std::size_t GetHash() const { return m_Hash; }

private:
    inline static uint32_t ms_RootSigID = 0;

    ComPtr<ID3D12RootSignature>          m_RootSignature;
    uint32_t                             m_ShaderRegisters[D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER + 1] = {};
    std::vector<CD3DX12_ROOT_PARAMETER1> m_RootParams;
    std::size_t                          m_Hash = 0;

    friend class GfxRootSignatureManager;
};

struct DefaultRootSignatures
{
    inline static GfxRootSignature DefaultGraphicsRootSignature = {};
};

class GfxRootSignatureManager
{
public:
    DeclareSingletonFunctions(GfxRootSignatureManager);

    void Initialize();
};
