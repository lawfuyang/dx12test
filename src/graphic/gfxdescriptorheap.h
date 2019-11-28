#pragma once

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
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType       = {};
    GfxDescriptorHeapFlags     m_HeapFlags      = {};
    uint32_t                   m_ManagerPoolIdx = 0xDEADBEEF; // to be filled by GfxDescriptorHeapManager
    uint32_t                   m_PoolIdx        = 0xDEADBEEF; // to be filled by GfxDescriptorHeapPool
    uint32_t                   m_PageIdx        = 0xDEADBEEF; // to be filled by GfxDescriptorHeapPageCommon

    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle = { 0 };
    D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle = { 0 };
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorHeapPageCommon
{
public:
    virtual ~GfxDescriptorHeapPageCommon() {}

    ID3D12DescriptorHeap* Dev() const { return m_DescriptorHeap.Get(); }

    virtual void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) = 0;
    virtual void Allocate(D3D12_DESCRIPTOR_HEAP_TYPE, GfxDescriptorHeapFlags, GfxDescriptorHeapHandle&, bool& pageIsFull) = 0;

protected:
    static constexpr uint32_t NbDescHandles = 32;

    void InitializeCommon(D3D12_DESCRIPTOR_HEAP_TYPE, D3D12_DESCRIPTOR_HEAP_FLAGS);
    void AllocateCommon(D3D12_DESCRIPTOR_HEAP_TYPE, GfxDescriptorHeapFlags, GfxDescriptorHeapHandle&, bool& pageIsFull);

    ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
    uint32_t m_DescriptorSize = 0;

    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, NbDescHandles> m_CPUDescriptorHandles;

    boost::lockfree::stack<uint32_t, boost::lockfree::capacity<NbDescHandles>> m_FreeHandlesIdx;
    std::array<bool, NbDescHandles> m_ActiveHandlesIdx; // TODO: make this thread safe
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorNonShaderVisibleHeapPage : public GfxDescriptorHeapPageCommon
{
public:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) override;
    void Allocate(D3D12_DESCRIPTOR_HEAP_TYPE, GfxDescriptorHeapFlags, GfxDescriptorHeapHandle&, bool& pageIsFull) override;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorShaderVisibleHeapPage : public GfxDescriptorHeapPageCommon
{
public:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) override;
    void Allocate(D3D12_DESCRIPTOR_HEAP_TYPE, GfxDescriptorHeapFlags, GfxDescriptorHeapHandle&, bool& pageIsFull) override;

private:
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, NbDescHandles> m_GPUDescriptorHandles;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
class GfxDescriptorHeapPoolBase
{
public:
    virtual ~GfxDescriptorHeapPoolBase() {}

    virtual void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) = 0;
    virtual GfxDescriptorHeapHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE) = 0;
};

//------------------------------------------------------------------------------------------------------------------------------------------------------
template <GfxDescriptorHeapFlags HeapFlags>
class GfxDescriptorHeapPool : public GfxDescriptorHeapPoolBase
{
public:
    void Initialize(D3D12_DESCRIPTOR_HEAP_TYPE) override;
    GfxDescriptorHeapHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE) override;

private:
    void AllocatePage(D3D12_DESCRIPTOR_HEAP_TYPE heapType);

    template <GfxDescriptorHeapFlags> struct GfxDescriptorHeapPoolType;
    template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapFlags::Static>        { using Type = GfxDescriptorNonShaderVisibleHeapPage; };
    template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapFlags::ShaderVisible> { using Type = GfxDescriptorShaderVisibleHeapPage; };
    template <> struct GfxDescriptorHeapPoolType<GfxDescriptorHeapFlags::Null>          { using Type = GfxDescriptorNonShaderVisibleHeapPage; };

    using GfxDescriptorHeapPageType = typename GfxDescriptorHeapPoolType<HeapFlags>::Type;

    boost::object_pool<GfxDescriptorHeapPageType> m_PagePool;
    SpinLock m_PoolLock;

    // TODO: read-write lock these
    boost::container::small_vector<GfxDescriptorHeapPageType*, 16> m_AvailablePages;
    boost::container::small_vector<uint32_t, 16> m_FreePagesIdx;
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

    // TODO: implement free

private:
    using PoolKeyType = uint32_t;
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
