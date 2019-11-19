#pragma once

class GfxPipelineState
{
public:
    ID3D12PipelineState* Dev() const { return m_PipelineState.Get(); }

private:
    ComPtr<ID3D12PipelineState> m_PipelineState;
};
