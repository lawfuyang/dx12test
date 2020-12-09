#include <graphic/gfx/gfxdescriptorheap.h>
#include <graphic/pch.h>

// using d3d12 enums for array idx
static_assert(D3D12_DESCRIPTOR_HEAP_FLAG_NONE == 0);
static_assert(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE == 1);
static_assert(GfxGPUDescriptorAllocator::NbDescHeapContexts == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE + 1);

void GfxDescriptorHeap::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    assert(!m_DescriptorHeap);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = numHeaps;
    heapDesc.Type = heapType;
    heapDesc.Flags = heapFlags;
    heapDesc.NodeMask = 0;

    DX12_CALL(gfxDevice.Dev()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));
}

void GfxGPUDescriptorAllocator::Initialize()
{
    for (uint32_t i = 0; i < NbDescHeapContexts; ++i)
    {
        m_InternalDescriptorHeap[i].Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, (D3D12_DESCRIPTOR_HEAP_FLAGS)i, NbDescriptors);

        static const uint32_t descHeapSize = g_GfxManager.GetGfxDevice().Dev()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        m_FreeHeaps[i].set_capacity(NbDescriptors);
        for (uint32_t j = 0; j < NbDescriptors; ++j)
        {
            GfxDescriptorHeapHandle newHandle{};
            newHandle.m_CPUHandle.InitOffsetted(m_InternalDescriptorHeap[i].Dev()->GetCPUDescriptorHandleForHeapStart(), j, descHeapSize);

            if (i == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
                newHandle.m_GPUHandle.InitOffsetted(m_InternalDescriptorHeap[i].Dev()->GetGPUDescriptorHandleForHeapStart(), j, descHeapSize);

            m_FreeHeaps[i].push_back(newHandle);
        }
    }
}

GfxDescriptorHeapHandle GfxGPUDescriptorAllocator::AllocateVolatile(D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps, GfxDescriptorHeapHandle* out)
{
    assert(numHeaps > 0);

    bbeAutoLock(m_GfxGPUDescriptorAllocatorLock[heapFlags]);

    // Requested shader visible heaps are assumed to be needed to be contiguous
    // In the event that we do a full circle around the CircularBuffer, and the number of requested heaps overflows back into the start, simply allocate and discard the tail heaps
    if ((m_AllocationCounter[heapFlags] + numHeaps) > NbDescriptors)
    {
        const int32_t allocationOverflow = NbDescriptors - m_AllocationCounter[heapFlags];
        AllocateVolatileInternal(heapFlags, allocationOverflow, nullptr);
    }

    return AllocateVolatileInternal(heapFlags, numHeaps, out);
}

GfxDescriptorHeapHandle GfxGPUDescriptorAllocator::AllocateVolatileInternal(D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps, GfxDescriptorHeapHandle* out)
{
    assert(m_FreeHeaps[heapFlags].size() >= numHeaps);

    // return only the first descriptor
    GfxDescriptorHeapHandle ret = m_FreeHeaps[heapFlags].front();

    for (uint32_t i = 0; i < numHeaps; ++i)
    {
        GfxDescriptorHeapHandle nextDesc = m_FreeHeaps[heapFlags].front();
        assert(nextDesc.m_CPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
        assert(nextDesc.m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        m_AllocationCounter[heapFlags] = ++m_AllocationCounter[heapFlags] % NbDescriptors;
        m_UsedHeaps[heapFlags].push_back(nextDesc);
        m_FreeHeaps[heapFlags].pop_front();

        if (out)
        {
            out[i] = nextDesc;
        }
    }

    return ret;
}

void GfxGPUDescriptorAllocator::GarbageCollect()
{
    bbeMultiThreadDetector();

    for (uint32_t i = 0; i < NbDescHeapContexts; ++i)
    {
        std::for_each(m_UsedHeaps[i].begin(), m_UsedHeaps[i].end(), [&](const GfxDescriptorHeapHandle& handle)
            {
                m_FreeHeaps[i].push_back(handle);
            });
        m_UsedHeaps[i].clear();
    }
}
