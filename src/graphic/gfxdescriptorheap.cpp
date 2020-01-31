#include "graphic/gfxdescriptorheap.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

//------------------------------------------------------------------------------------------------------------------------------------------------------
void GfxDescriptorHeap::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags)
{
    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    m_DescHeapHandle = gfxDevice.GetDescriptorHeapManager().Allocate(heapType, heapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE ? GfxDescriptorHeapFlags::ShaderVisible : GfxDescriptorHeapFlags::Static);
    assert(m_DescHeapHandle.m_ManagerPoolIdx != 0xDEADBEEF);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void GfxDescriptorHeapPageCommon::InitializeCommon(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    assert(m_DescriptorHeap.Get() == nullptr);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = NbDescHandles;
    heapDesc.Type = heapType;
    heapDesc.Flags = flags;
    heapDesc.NodeMask = 0; // For single-adapter, set to 0. Else, set a bit to identify the node

    DX12_CALL(gfxDevice.Dev()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

    m_DescriptorSize = gfxDevice.Dev()->GetDescriptorHandleIncrementSize(heapType);

    for (uint32_t i = 0; i < NbDescHandles; ++i)
    {
        m_CPUDescriptorHandles[i].ptr = m_DescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_DescriptorSize * i;
        m_FreeHandlesIdx.push(NbDescHandles - i - 1); // since its a stack, init descendingly
        m_ActiveHandlesIdx[i] = false;
    }
}

void GfxDescriptorHeapPageCommon::AllocateCommon(D3D12_DESCRIPTOR_HEAP_TYPE heapType, GfxDescriptorHeapFlags heapFlags, GfxDescriptorHeapHandle& newHandle, bool& pageIsFull)
{
    newHandle.m_HeapType = heapType;
    newHandle.m_HeapFlags = heapFlags;
    
    // take first free list
    m_FreeHandlesIdx.consume_one([&](uint32_t freeIdx)
        {
            m_ActiveHandlesIdx[freeIdx] = true;
            newHandle.m_PageIdx = freeIdx;
            newHandle.m_CPUHandle = m_CPUDescriptorHandles[freeIdx];
        });

    // need to tell GfxDescriptorHeapPool if theres no more free heaps, so that it can allocate a new page
    pageIsFull = m_FreeHandlesIdx.empty();
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void GfxDescriptorNonShaderVisibleHeapPage::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    InitializeCommon(type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
}

void GfxDescriptorNonShaderVisibleHeapPage::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType, GfxDescriptorHeapFlags heapFlags, GfxDescriptorHeapHandle& newHandle, bool& pageIsFull)
{
    AllocateCommon(heapType, heapFlags, newHandle, pageIsFull);
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void GfxDescriptorShaderVisibleHeapPage::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    InitializeCommon(type, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    for (uint32_t i = 0; i < NbDescHandles; ++i)
    {
        m_GPUDescriptorHandles[i].ptr = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + m_DescriptorSize * i;
    }
}

void GfxDescriptorShaderVisibleHeapPage::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType, GfxDescriptorHeapFlags heapFlags, GfxDescriptorHeapHandle& newHandle, bool& pageIsFull)
{
    AllocateCommon(heapType, heapFlags, newHandle, pageIsFull);

    // need to init GPU Desc Handle for shader visible desc heaps
    newHandle.m_GPUHandle = m_GPUDescriptorHandles[newHandle.m_PageIdx];
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
void GfxDescriptorHeapManager::Initialize()
{
    bbeProfileFunction();

    m_PoolsMap[ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GfxDescriptorHeapFlags::Static)]        = &m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    m_PoolsMap[ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GfxDescriptorHeapFlags::Static)]            = &m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];
    m_PoolsMap[ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, GfxDescriptorHeapFlags::Static)]                = &m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
    m_PoolsMap[ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, GfxDescriptorHeapFlags::Static)]                = &m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_DSV];
    m_PoolsMap[ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GfxDescriptorHeapFlags::ShaderVisible)] = &m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    m_PoolsMap[ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GfxDescriptorHeapFlags::ShaderVisible)]     = &m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];
    m_PoolsMap[ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GfxDescriptorHeapFlags::Null)]          = &m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    m_PoolsMap[ToPoolKey(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GfxDescriptorHeapFlags::Null)]              = &m_StaticPools[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];

    for (auto& pool : m_PoolsMap)
    {
        const PoolKeyType poolKey = pool.first;
        GfxDescriptorHeapPoolBase* poolBase = pool.second;

        poolBase->Initialize(ToHeapType(poolKey));
    }
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
GfxDescriptorHeapHandle GfxDescriptorHeapManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType, GfxDescriptorHeapFlags heapFlags)
{
    const uint32_t poolIdx = ToPoolKey(heapType, heapFlags);

    GfxDescriptorHeapPoolBase* poolToAllocateFrom = m_PoolsMap.at(poolIdx);
    assert(poolToAllocateFrom);

    GfxDescriptorHeapHandle newHandle = poolToAllocateFrom->Allocate(heapType);
    assert(newHandle.m_PoolIdx != 0xDEADBEEF);

    newHandle.m_ManagerPoolIdx = poolIdx;

    return newHandle;
}
