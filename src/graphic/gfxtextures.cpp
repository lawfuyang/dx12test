#include "graphic/gfxtextures.h"

#include "graphic/gfxdevice.h"

D3D12_CPU_DESCRIPTOR_HANDLE GfxRenderTargetView::GetCPUDescHandle() const
{
    return D3D12_CPU_DESCRIPTOR_HANDLE{ m_DescHeap.Dev()->GetCPUDescriptorHandleForHeapStart() };
}

void GfxRenderTargetView::Initialize(GfxDevice& gfxDevice, ID3D12Resource* resource)
{
    bbeProfileFunction();

    m_Resource = resource;

    m_DescHeap.Initialize(gfxDevice, 1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    // TODO: Fill In
    //D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    //rtvDesc.Format = ;
    //rtvDesc.ViewDimension = ;
    //rtvDesc.Texture2D.MipSlice = ;
    //rtvDesc.Texture2D.PlaneSlice = ;

    gfxDevice.Dev()->CreateRenderTargetView(m_Resource.Get(), nullptr, m_DescHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}
