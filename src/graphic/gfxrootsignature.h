#pragma once

class GfxRootSignature
{
public:
    ID3D12RootSignature* Dev() const { return m_RootSignature.Get(); }

    void AddSRV();
    void Compile();

    uint32_t GetID() const { return ms_RootSigID; }

private:
    inline static std::atomic<uint32_t> ms_RootSigID = 0;

    ComPtr<ID3D12RootSignature> m_RootSignature;

    uint32_t m_ShaderRegisters[D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER + 1] = {};
    std::vector<CD3DX12_ROOT_PARAMETER1> m_RootParams;
};
static GfxRootSignature g_DefaultGraphicsRootSignature;

class GfxRootSignatureManager
{
public:
    DeclareSingletonFunctions(GfxRootSignatureManager);

    void Initialize();
};
