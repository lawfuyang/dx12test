#pragma once

class GfxRootSignature
{
public:
    ID3D12RootSignature* Dev() const { return m_RootSignature.Get(); }

    void AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE, uint32_t numDescriptors, uint32_t baseShaderRegister);
    void Compile(const std::string& rootSigName);

    std::size_t GetHash() const { return m_Hash; }

private:
    ComPtr<ID3D12RootSignature>            m_RootSignature;
    std::vector<CD3DX12_DESCRIPTOR_RANGE1> m_DescRanges;
    std::size_t                            m_Hash = 0;

    friend class GfxRootSignatureManager;
};

struct DefaultRootSignatures
{
    inline static GfxRootSignature DefaultGraphicsRootSignature;
};

class GfxRootSignatureManager
{
public:
    DeclareSingletonFunctions(GfxRootSignatureManager);

    void Initialize();
};
#define g_GfxRootSignatureManager GfxRootSignatureManager::GetInstance()
