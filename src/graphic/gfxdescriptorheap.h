#pragma once

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorHeapPageCommon
{
public:
    virtual ~GfxDescriptorHeapPageCommon() {}

    ID3D12DescriptorHeap* Dev() const { return m_DescriptorHeap.Get(); }

protected:
    static constexpr uint32_t NbDescHandles = 32;

    void InitializeCommon(D3D12_DESCRIPTOR_HEAP_TYPE, D3D12_DESCRIPTOR_HEAP_FLAGS);

    ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
    uint32_t m_DescriptorSize = 0;

    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, NbDescHandles> m_CPUDescriptorHandles;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorNonShaderVisibleHeapPage : public GfxDescriptorHeapPageCommon
{
public:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE);
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorShaderVisibleHeapPage : public GfxDescriptorHeapPageCommon
{
public:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE);

private:
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, NbDescHandles> m_GPUDescriptorHandles;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
enum class GfxDescriptorHeapType
{
    Static,
    ShaderVisible,
    Null,
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
template <GfxDescriptorHeapType> struct GfxDescriptorHeapPoolType;
template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapType::Static>        { using Type = GfxDescriptorNonShaderVisibleHeapPage; };
template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapType::ShaderVisible> { using Type = GfxDescriptorShaderVisibleHeapPage; };
template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapType::Null>          { using Type = GfxDescriptorNonShaderVisibleHeapPage; };

template <GfxDescriptorHeapType PoolType>
using GfxDescriptorHeapPoolType_v = typename GfxDescriptorHeapPoolType<PoolType>::Type;

//------------------------------------------------------------------------------------------------------------------------------------------------------
template <GfxDescriptorHeapType PoolType>
class GfxDescriptorHeapPool
{
public:
    void Initialize();

private:
    GfxDescriptorHeapPoolType_v<PoolType> m_Pool;
};
using GfxStaticDescriptorHeapPool   = GfxDescriptorHeapPool<GfxDescriptorHeapType::Static>;
using GfxVolatileDescriptorHeapPool = GfxDescriptorHeapPool<GfxDescriptorHeapType::ShaderVisible>;
using GfxNullDescriptorHeapPool     = GfxDescriptorHeapPool<GfxDescriptorHeapType::Null>;

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorHeapHandle
{
public:
    explicit GfxDescriptorHeapHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
        : m_CPUHandle(cpuHandle)
        , m_GPUHandle(gpuHandle)
    {}

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_CPUHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const { return m_GPUHandle; }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle = {};
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorHeapManager
{
public:
    void Initalize();

    GfxDescriptorHeapHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags);
    void Free(GfxDescriptorHeapHandle);

private:
    GfxStaticDescriptorHeapPool   m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    GfxVolatileDescriptorHeapPool m_ShaderVisiblePools[2]; // CBV_SRV_UAV & SAMPLER
    GfxNullDescriptorHeapPool     m_NullPools[2]; // CBV_SRV_UAV & SAMPLER
};
static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == 0);
static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER == 1);

#include "graphic/gfxdescriptorheap.hpp"
