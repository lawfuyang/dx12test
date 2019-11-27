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
enum class GfxDescriptorHeapFlags
{
    Static,
    ShaderVisible,
    Null,
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
struct GfxDescriptorHeapHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle = {};
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorHeapPoolBase
{
public:
    virtual ~GfxDescriptorHeapPoolBase() {}

    virtual void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) = 0;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
template <GfxDescriptorHeapFlags HeapFlags>
class GfxDescriptorHeapPool : public GfxDescriptorHeapPoolBase
{
public:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) override;

private:
    template <GfxDescriptorHeapFlags> struct GfxDescriptorHeapPoolType;
    template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapFlags::Static>        { using Type = GfxDescriptorNonShaderVisibleHeapPage; };
    template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapFlags::ShaderVisible> { using Type = GfxDescriptorShaderVisibleHeapPage; };
    template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapFlags::Null>          { using Type = GfxDescriptorNonShaderVisibleHeapPage; };

    using GfxDescriptorHeapPageType = typename GfxDescriptorHeapPoolType<HeapFlags>::Type;

    boost::object_pool<GfxDescriptorHeapPageType> m_Pool;
    SpinLock m_PoolLock;

    std::vector<GfxDescriptorHeapPageType*> m_AvailablePages;
    std::vector<GfxDescriptorHeapPageType*> m_FullPages;
    SpinLock m_PagesLock;
};
using GfxStaticDescriptorHeapPool   = GfxDescriptorHeapPool<GfxDescriptorHeapFlags::Static>;
using GfxVolatileDescriptorHeapPool = GfxDescriptorHeapPool<GfxDescriptorHeapFlags::ShaderVisible>;
using GfxNullDescriptorHeapPool     = GfxDescriptorHeapPool<GfxDescriptorHeapFlags::Null>;

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorHeapManager
{
public:
    void Initalize();

    GfxDescriptorHeapHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType, GfxDescriptorHeapFlags heapFlags);
    void Free(GfxDescriptorHeapHandle);

private:
    using PoolKeyType = uint16_t;
    std::unordered_map<PoolKeyType, GfxDescriptorHeapPoolBase*> m_PoolsMap;

    constexpr PoolKeyType ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE heapType, GfxDescriptorHeapFlags heapFlags) const
    {
        return (heapType << 8) | (uint8_t)heapFlags;
    }

    constexpr D3D12_DESCRIPTOR_HEAP_TYPE ToHeapType(PoolKeyType key) const
    {
        return (D3D12_DESCRIPTOR_HEAP_TYPE)(key >> 8);
    }

    GfxStaticDescriptorHeapPool   m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    GfxVolatileDescriptorHeapPool m_ShaderVisiblePools[2]; // CBV_SRV_UAV & SAMPLER
    GfxNullDescriptorHeapPool     m_NullPools[2]; // CBV_SRV_UAV & SAMPLER
};
static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV == 0);
static_assert(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER == 1);

#include "graphic/gfxdescriptorheap.hpp"
