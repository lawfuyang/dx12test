#pragma once

class GfxRootSignature
{
public:
    void Initialize();

private:
    ComPtr<ID3D12RootSignature> m_RootSignature;
};
