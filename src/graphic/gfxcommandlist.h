#pragma once

class GfxDevice;

class GfxCommandList
{
public:
    ID3D12GraphicsCommandList* Dev() const { return m_CommandList.Get(); }

    void Initialize(GfxDevice&, D3D12_COMMAND_LIST_TYPE);

private:
    ComPtr<ID3D12GraphicsCommandList> m_CommandList;
};
