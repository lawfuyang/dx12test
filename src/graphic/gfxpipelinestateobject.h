#pragma once

class GfxPSOManager
{
public:
    ID3D12PipelineState* GetPSO(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& graphicPSO);

private:
    std::unordered_map<std::uint64_t, ComPtr<ID3D12PipelineState>> m_PSOMap;
};
