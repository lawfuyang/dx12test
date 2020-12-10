#include <graphic/gfx/gfxdescriptorheap.h>
#include <graphic/pch.h>

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

    // allocate static heaps
    m_StaticHeaps.set_capacity(NbStaticDescriptors);
    for (uint32_t i = 0; i < NbStaticDescriptors; ++i)
    {
        m_StaticHeaps.push_back();
    }
}

GfxDescriptorHeapHandle GfxGPUDescriptorAllocator::AllocateShaderVisible(uint32_t numHeaps, GfxDescriptorHeapHandle* out)
{
    assert(numHeaps > 0);

    static std::mutex ShaderVisibleDescHeapAllocatorLock;
    bbeAutoLock(ShaderVisibleDescHeapAllocatorLock);

    // Requested shader visible heaps are assumed to be needed to be contiguous
    // In the event that we do a full circle around the CircularBuffer, and the number of requested heaps overflows back into the start, simply allocate and discard the tail heaps
    if ((m_AllocationCounter + numHeaps) > NbShaderVisibleDescriptors)
    {
        const int32_t allocationOverflow = NbShaderVisibleDescriptors - m_AllocationCounter;
        AllocateShaderVisibleInternal(allocationOverflow, nullptr);
    }

    return AllocateShaderVisibleInternal(numHeaps, out);
}

GfxDescriptorHeapHandle GfxGPUDescriptorAllocator::AllocateShaderVisibleInternal(uint32_t numHeaps, GfxDescriptorHeapHandle* out)
{
    assert(m_FreeShaderVisibleHeaps.size() >= numHeaps);

    // return only the first descriptor
    GfxDescriptorHeapHandle ret = m_FreeShaderVisibleHeaps.front();

    for (uint32_t i = 0; i < numHeaps; ++i)
    {
        GfxDescriptorHeapHandle nextDesc = m_FreeShaderVisibleHeaps.front();
        assert(nextDesc.m_CPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
        assert(nextDesc.m_GPUHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

        m_AllocationCounter = ++m_AllocationCounter % NbShaderVisibleDescriptors;
        m_UsedHeaps.push_back(nextDesc);
        m_FreeShaderVisibleHeaps.pop_front();

        if (out)
        {
            out[i] = nextDesc;
        }
    }

    return ret;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE GfxGPUDescriptorAllocator::AllocateStatic(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    static std::mutex StaticDescHeapAllocatorLock;
    bbeAutoLock(StaticDescHeapAllocatorLock);

    // Get a heap from the front of buffer & re-init it to appropriate type
    GfxDescriptorHeap& toRet = m_StaticHeaps.front();
    toRet.Release();
    toRet.Initialize(type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

    // rotate circular buffer to the next element, so we always use the "oldest" heap
    assert(m_StaticHeaps.full());
    m_StaticHeaps.rotate(m_StaticHeaps.begin() + 1);

    return CD3DX12_CPU_DESCRIPTOR_HANDLE{ toRet.Dev()->GetCPUDescriptorHandleForHeapStart() };
}

void GfxGPUDescriptorAllocator::GarbageCollect()
{
    bbeMultiThreadDetector();

    std::for_each(m_UsedHeaps.begin(), m_UsedHeaps.end(), [&](const GfxDescriptorHeapHandle& handle)
        {
            m_FreeShaderVisibleHeaps.push_back(handle);
        });
    m_UsedHeaps.clear();
}
