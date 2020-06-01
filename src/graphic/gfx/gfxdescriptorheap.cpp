#include "graphic/gfx/gfxdescriptorheap.h"

#include "graphic/gfx/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfx/gfxdevice.h"

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
    m_InternalDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, NumDynamicDescriptors);

    const uint32_t descHeapSize = g_GfxManager.GetGfxDevice().Dev()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    m_FreeHeaps.set_capacity(NumDynamicDescriptors);
    for (uint32_t i = 0; i < NumDynamicDescriptors; ++i)
    {
        GfxDescriptorHeapHandle newHandle;
        newHandle.m_CPUHandle.InitOffsetted(m_InternalDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart(), i, descHeapSize);
        newHandle.m_GPUHandle.InitOffsetted(m_InternalDescriptorHeap.Dev()->GetGPUDescriptorHandleForHeapStart(), i, descHeapSize);
        m_FreeHeaps.push_back(newHandle);
    }
}

GfxDescriptorHeapHandle GfxGPUDescriptorAllocator::Allocate(uint32_t numHeaps, GfxDescriptorHeapHandle* out)
{
    assert(numHeaps > 0);

    bbeAutoLock(m_GfxGPUDescriptorAllocatorLock);

    // Requested shader visible heaps are assumed to be needed to be contiguous
    // In the event that we do a full circle around the CircularBuffer, and the number of requested heaps overflows back into the start, simply allocate and discard the tail heaps
    if (const int32_t allocationOverflow = (int32_t)(m_AllocationCounter + numHeaps) - NumDynamicDescriptors;
        allocationOverflow > 0)
    {
        AllocateInternal(allocationOverflow, nullptr);
    }

    return AllocateInternal(numHeaps, out);
}

GfxDescriptorHeapHandle GfxGPUDescriptorAllocator::AllocateInternal(uint32_t numHeaps, GfxDescriptorHeapHandle* out)
{
    assert(m_FreeHeaps.size() >= numHeaps);

    // return only the first descriptor
    GfxDescriptorHeapHandle ret = m_FreeHeaps.front();

    for (uint32_t i = 0; i < numHeaps; ++i)
    {
        GfxDescriptorHeapHandle nextDesc = m_FreeHeaps.front();
        assert(nextDesc.m_CPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
        assert(nextDesc.m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        m_AllocationCounter = ++m_AllocationCounter % NumDynamicDescriptors;
        m_UsedHeaps.push_back(nextDesc);
        m_FreeHeaps.pop_front();

        if (out)
        {
            out[i] = nextDesc;
        }
    }

    return ret;
}

void GfxGPUDescriptorAllocator::CleanupUsedHeaps()
{
    bbeMultiThreadDetector();

    std::for_each(m_UsedHeaps.begin(), m_UsedHeaps.end(), [&](const GfxDescriptorHeapHandle& handle)
        {
            m_FreeHeaps.push_back(handle);
        });
    m_UsedHeaps.clear();
}
