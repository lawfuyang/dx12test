#include "graphic/gfxtexturesandbuffers.h"

#include "graphic/gfxmanager.h"
#include "graphic/gfxdevice.h"
#include "graphic/dx12utils.h"

void GfxHazardTrackedResource::Transition(GfxCommandList& cmdList, D3D12_RESOURCE_STATES newState)
{
    if (m_CurrentResourceState != newState)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_Resource.Get();
        barrier.Transition.StateBefore = m_CurrentResourceState;
        barrier.Transition.StateAfter = newState;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        const UINT numBarriers = 1;
        cmdList.Dev()->ResourceBarrier(numBarriers, &barrier);
        m_CurrentResourceState = newState;
    }
}

void GfxResourceView::InitializeCommon(ID3D12Resource* resource, D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    m_HazardTrackedResource.Set(resource);

    m_DescHeapHandle = gfxDevice.GetDescriptorHeapManager().Allocate(heapType, heapFlags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE ? GfxDescriptorHeapFlags::ShaderVisible : GfxDescriptorHeapFlags::Static);
    assert(m_DescHeapHandle.m_ManagerPoolIdx != 0xDEADBEEF);
}

void GfxRenderTargetView::Initialize(ID3D12Resource* resource)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    InitializeCommon(resource, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    // TODO: Fill In
    //D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    //rtvDesc.Format = ;
    //rtvDesc.ViewDimension = ;
    //rtvDesc.Texture2D.MipSlice = ;
    //rtvDesc.Texture2D.PlaneSlice = ;

    {
        bbeProfile("CreateRenderTargetView");
        gfxDevice.Dev()->CreateRenderTargetView(m_HazardTrackedResource.Dev(), nullptr, m_DescHeapHandle.m_CPUHandle);
    }
}

void GfxShaderResourceView::Initialize(ID3D12Resource* resource)
{
    bbeProfileFunction();

    InitializeCommon(resource, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
}
