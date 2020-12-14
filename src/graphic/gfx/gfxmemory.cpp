#include <graphic/gfx/gfxmemory.h>
#include <graphic/pch.h>

void GfxMemoryAllocator::Initialize()
{
    static bool s_AlwaysCommitedMemory = false;

    D3D12MA::ALLOCATOR_DESC desc{};
    desc.Flags = s_AlwaysCommitedMemory ? D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED : D3D12MA::ALLOCATOR_FLAG_NONE;
    desc.pDevice = g_GfxManager.GetGfxDevice().Dev();
    desc.pAdapter = g_GfxAdapter.GetAllAdapters()[0].Get(); // Just get first adapter

    DX12_CALL(D3D12MA::CreateAllocator(&desc, &m_D3D12MemoryAllocator));
    assert(m_D3D12MemoryAllocator);
}

D3D12MA::Allocation* GfxMemoryAllocator::AllocateInternal(D3D12_HEAP_TYPE heapType, const CD3DX12_RESOURCE_DESC1& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
    bbeProfileFunction();

    D3D12MA::ALLOCATION_DESC bufferAllocDesc{ D3D12MA::ALLOCATION_FLAG_WITHIN_BUDGET, heapType };

    static ID3D12ProtectedResourceSession* ProtectedSession = nullptr; // TODO
    D3D12MA::Allocation* allocHandle = nullptr;
    D3D12Resource* newHeap = nullptr;
    DX12_CALL(Dev().CreateResource2(
        &bufferAllocDesc,
        &desc,
        initialState,
        clearValue,
        ProtectedSession,
        &allocHandle,
        IID_PPV_ARGS(&newHeap)));

    assert(allocHandle);
    assert(newHeap);

    return allocHandle;
}

D3D12MA::Allocation* GfxMemoryAllocator::AllocateStatic(const CD3DX12_RESOURCE_DESC1& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
    D3D12MA::Allocation* allocHandle = AllocateInternal(D3D12_HEAP_TYPE_DEFAULT, desc, initialState, clearValue);

    bbeAutoLock(m_StaticAllocationsLck);
    m_StaticAllocations.insert(allocHandle);

    return allocHandle;
}

// TODO: Heap Pools of 256 bytes, 4 KB & 64 KB
D3D12MA::Allocation* GfxMemoryAllocator::AllocateVolatile(D3D12_HEAP_TYPE heapType, const CD3DX12_RESOURCE_DESC1& desc)
{
    assert(heapType == D3D12_HEAP_TYPE_UPLOAD || heapType == D3D12_HEAP_TYPE_READBACK);

    // TODO: initial state for proper heapTypes?
    const D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

    D3D12MA::Allocation* allocHandle = AllocateInternal(heapType, desc, initialState, nullptr);

    bbeAutoLock(m_VolatileAllocationsLck);
    m_VolatileAllocations.push_back(allocHandle);

    return allocHandle;
}

void GfxMemoryAllocator::ShutDown()
{
    GarbageCollect();

    for (D3D12MA::Allocation* allocHandle : m_StaticAllocations)
    {
        allocHandle->Release();
    }
    
    Dev().Release();
}

void GfxMemoryAllocator::ReleaseStatic(D3D12MA::Allocation* allocation)
{
    allocation->GetResource()->Release();
    allocation->Release();

    bbeAutoLock(m_StaticAllocationsLck);
    m_StaticAllocations.erase(allocation);
}

// TODO: use Fences to release them conditionally when app no longer waits for master signal for main queue
void GfxMemoryAllocator::GarbageCollect()
{
    bbeAutoLock(m_VolatileAllocationsLck);
    for (D3D12MA::Allocation* allocHandle : m_VolatileAllocations)
    {
        allocHandle->GetResource()->Release();
        allocHandle->Release();
    }
    m_VolatileAllocations.clear();
}
