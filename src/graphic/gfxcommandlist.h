#pragma once

class GfxDevice;
class GfxPipelineState;

class GfxCommandList
{
public:
    ID3D12GraphicsCommandList* Dev() const { return m_CommandList.Get(); }

    void Initialize(GfxDevice&, D3D12_COMMAND_LIST_TYPE);

    void BeginRecording();

private:
    ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_CommandList;
};
