#pragma once

#include <graphic/dx12utils.h>

namespace D3D12MA
{
    class Allocation;
};

class GfxCommandList;
class GfxContext;

class GfxHazardTrackedResource
{
public:
    static const D3D12_RESOURCE_STATES INVALID_STATE = (D3D12_RESOURCE_STATES)0xDEADBEEF;

    D3D12Resource* GetD3D12Resource() const { return m_D3D12Resource; }
    void SetDebugName(std::string_view debugName) const;
    void Release();

    D3D12MA::Allocation* m_D3D12MABufferAllocation = nullptr;
    D3D12Resource*       m_D3D12Resource           = nullptr;

    D3D12_RESOURCE_STATES m_CurrentResourceState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES m_TransitioningState   = INVALID_STATE;

    friend class GfxContext;
};

class GfxVertexBuffer : public GfxHazardTrackedResource
{
public:
    void Initialize(uint32_t numVertices, uint32_t vertexSize, const void* initData = nullptr);

    uint32_t GetStrideInBytes() const { return m_StrideInBytes; }
    uint32_t GetNumVertices() const { return m_NumVertices; }

    uint32_t m_StrideInBytes = 0;
    uint32_t m_NumVertices = 0;
};

class GfxIndexBuffer : public GfxHazardTrackedResource
{
public:
    void Initialize(uint32_t numIndices, const void* initData = nullptr);

    DXGI_FORMAT GetFormat() const { DXGI_FORMAT_R16_UINT; } // only support 16 bit indices for now
    uint32_t GetNumIndices() const { return m_NumIndices; }

    uint32_t m_NumIndices = 0;
};

class GfxTexture : public GfxHazardTrackedResource
{
public:
    DeclareObjectModelFunctions(GfxTexture);

    void Initialize(const CD3DX12_RESOURCE_DESC1& desc, const void* initData = nullptr, D3D12_CLEAR_VALUE clearValue = D3D12_CLEAR_VALUE{}, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ);

    DXGI_FORMAT GetFormat() const { return m_Format; }

    DXGI_FORMAT m_Format = DXGI_FORMAT_UNKNOWN;

    void UpdateIMGUI();
};
