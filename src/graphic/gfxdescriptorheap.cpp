#include "graphic/gfxdescriptorheap.h"

#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxDescriptorHeap::Initialize(GfxDevice& gfxDevice, uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
    bbeProfileFunction();

    bbeAssert(m_DescriptorHeap.Get() == nullptr, "");

    // Describe and create a render target view (RTV) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = heapType;
    heapDesc.Flags = flags;
    heapDesc.NodeMask = 0; // For single-adapter, set to 0. Else, set a bit to identify the node

    DX12_CALL(gfxDevice.Dev()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));

    m_DescriptorSize = gfxDevice.Dev()->GetDescriptorHandleIncrementSize(heapType);
}
