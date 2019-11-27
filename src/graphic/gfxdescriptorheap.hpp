#pragma once

template <GfxDescriptorHeapFlags HeapFlags>
void GfxDescriptorHeapPool<HeapFlags>::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
    // init some free desc heaps
    GfxDescriptorHeapPageType* newPage = m_Pool.construct();
    assert(newPage);

    newPage->Initialize(heapType);
    m_AvailablePages.push_back(newPage);
}
