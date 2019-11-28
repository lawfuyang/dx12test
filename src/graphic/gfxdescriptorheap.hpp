#include "gfxdescriptorheap.h"
#pragma once

template <GfxDescriptorHeapFlags HeapFlags>
void GfxDescriptorHeapPool<HeapFlags>::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
    // init some one page full of free desc heaps
    AllocatePage(heapType);
}

template<GfxDescriptorHeapFlags HeapFlags>
inline GfxDescriptorHeapHandle GfxDescriptorHeapPool<HeapFlags>::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
    if (m_FreePagesIdx.empty())
    {
        AllocatePage(heapType);
    }

    const uint32_t freeIdx = m_FreePagesIdx[0];

    GfxDescriptorHeapPageType* pageWithFreeDescHeap = m_AvailablePages[freeIdx];
    GfxDescriptorHeapHandle newHandle;

    bool pageIsFull = false;
    pageWithFreeDescHeap->Allocate(heapType, HeapFlags, newHandle, pageIsFull);
    assert(newHandle.m_PageIdx != 0xDEADBEEF);
    assert(newHandle.m_CPUHandle.ptr != 0);

    if constexpr (HeapFlags == GfxDescriptorHeapFlags::ShaderVisible)
    {
        assert(newHandle.m_GPUHandle.ptr != 0);
    }

    newHandle.m_PoolIdx = freeIdx;

    if (pageIsFull)
    {
        AllocatePage(heapType);
    }

    return newHandle;
}

template<GfxDescriptorHeapFlags HeapFlags>
inline void GfxDescriptorHeapPool<HeapFlags>::AllocatePage(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
    // TODO: make this thread safe

    GfxDescriptorHeapPageType* newPage = m_PagePool.construct();
    assert(newPage);

    newPage->Initialize(heapType);
    m_FreePagesIdx.push_back(m_AvailablePages.size());
    m_AvailablePages.push_back(newPage);
}
