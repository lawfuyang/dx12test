#include "graphic/gfxdescriptorheap.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

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
    }
}

void GfxDescriptorHeapManager::Initalize()
{
    bbeProfileFunction();
}

void GfxDescriptorNonShaderVisibleHeapPage::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    bbeProfileFunction();

    InitializeCommon(type, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
}

void GfxDescriptorShaderVisibleHeapPage::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    bbeProfileFunction();

    InitializeCommon(type, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    for (uint32_t i = 0; i < NbDescHandles; ++i)
    {
        m_GPUDescriptorHandles[i].ptr = m_DescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + m_DescriptorSize * i;
    }
}