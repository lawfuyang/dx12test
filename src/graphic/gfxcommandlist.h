#pragma once

class GfxDevice;

class GfxCommandList
{
public:
    void Initialize(GfxDevice&, D3D12_COMMAND_LIST_TYPE);

private:
    ComPtr<ID3D12GraphicsCommandList> m_CommandList;
};
