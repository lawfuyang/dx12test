#include "graphic/gfxtextures.h"

#include "graphic/gfxdevice.h"

void GfxRenderTargetView::Initialize(GfxDevice& gfxDevice, ID3D12Resource* resource)
{
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
