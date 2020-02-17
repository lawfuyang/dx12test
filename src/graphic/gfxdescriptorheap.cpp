#include "graphic/gfxdescriptorheap.h"

#include "graphic/gfxmanager.h"
#include "graphic/dx12utils.h"
#include "graphic/gfxdevice.h"

void GfxDescriptorHeap::Initialize(D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    assert(!m_DescriptorHeap);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 1; // TODO: make an allocator to manage this?
    heapDesc.Type = heapType;
    heapDesc.Flags = heapFlags;
    heapDesc.NodeMask = 0;

    DX12_CALL(gfxDevice.Dev()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DescriptorHeap)));
}
