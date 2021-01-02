#include <graphic/gfx/gfxdescriptorheap.h>
#include <graphic/pch.h>

void GfxDescriptorHeap::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags, uint32_t numHeaps)
{
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
    bbeProfileFunction();

    m_ShaderVisibleDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, NbShaderVisibleDescriptors);

    static const uint32_t descHeapSize = g_GfxManager.GetGfxDevice().Dev()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // allocate shader visible heaps
    m_FreeShaderVisibleHeaps.set_capacity(NbShaderVisibleDescriptors);
    for (uint32_t i = 0; i < NbShaderVisibleDescriptors; ++i)
    {
        GfxDescriptorHeapHandle newHandle{};
        newHandle.m_CPUHandle.InitOffsetted(m_ShaderVisibleDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart(), i, descHeapSize);
        newHandle.m_GPUHandle.InitOffsetted(m_ShaderVisibleDescriptorHeap.Dev()->GetGPUDescriptorHandleForHeapStart(), i, descHeapSize);

        m_FreeShaderVisibleHeaps.push_back(newHandle);
    }
}

GfxDescriptorHeapHandle GfxGPUDescriptorAllocator::AllocateShaderVisible(uint32_t numHeaps)
{
    assert(numHeaps > 0);

    static std::mutex ShaderVisibleDescHeapAllocatorLock;
    bbeAutoLock(ShaderVisibleDescHeapAllocatorLock);

    // Requested shader visible heaps are assumed to be needed to be contiguous
    // In the event that we do a full circle around the CircularBuffer, and the number of requested heaps overflows back into the start, simply allocate and discard the tail heaps
    if ((m_AllocationCounter + numHeaps) > NbShaderVisibleDescriptors)
    {
        const int32_t allocationOverflow = NbShaderVisibleDescriptors - m_AllocationCounter;
        AllocateShaderVisibleInternal(allocationOverflow);
    }

    return AllocateShaderVisibleInternal(numHeaps);
}

GfxDescriptorHeapHandle GfxGPUDescriptorAllocator::AllocateShaderVisibleInternal(uint32_t numHeaps)
{
    assert(m_FreeShaderVisibleHeaps.size() >= numHeaps);
    m_AllocationCounter = (m_AllocationCounter + numHeaps) % NbShaderVisibleDescriptors;

    // allocate needed heaps and rotate circular buffer
    GfxDescriptorHeapHandle ret = m_FreeShaderVisibleHeaps.front();
    m_FreeShaderVisibleHeaps.rotate(m_FreeShaderVisibleHeaps.begin() + numHeaps);

    return ret;
}
