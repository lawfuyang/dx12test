#include "graphic/gfxtexturesandbuffers.h"

#include "graphic/gfxmanager.h"
#include "graphic/gfxdevice.h"
#include "graphic/gfxcontext.h"
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

void GfxRenderTargetView::Initialize(ID3D12Resource* resource, DXGI_FORMAT format)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    m_Resource = resource;
    m_Resource->SetName(L"Render Target View");

    {
        bbeProfile("CreateRenderTargetView");
        gfxDevice.Dev()->CreateRenderTargetView(m_Resource.Get(), nullptr, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
    }

    m_Format = format;
}

GfxBufferCommon::~GfxBufferCommon()
{
    bbeProfileFunction();

    if (m_D3D12MABufferAllocation)
    {
        g_Log.info("Destroying D3D12MA::Allocation '{}'", utf8_encode(m_D3D12MABufferAllocation->GetName()));

        m_D3D12MABufferAllocation->Release();
        m_D3D12MABufferAllocation = nullptr;
    }
}

void GfxBufferCommon::CreateHeap(GfxContext& context, D3D12_HEAP_TYPE heapType, const CD3DX12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState, D3D12MA::Allocation*& allocHandle)
{
    GfxDevice& gfxDevice = context.GetDevice();
    D3D12MA::Allocator* d3d12MemoryAllocator = gfxDevice.GetD3D12MemoryAllocator();

    D3D12MA::ALLOCATION_DESC vertexBufferAllocDesc = {};
    vertexBufferAllocDesc.HeapType = heapType;

    ID3D12Resource* newHeap;
    DX12_CALL(d3d12MemoryAllocator->CreateResource(
        &vertexBufferAllocDesc,
        &resourceDesc,
        initialState,
        nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
        &allocHandle,
        IID_PPV_ARGS(&newHeap)));
}

void GfxBufferCommon::UploadInitData(GfxContext& context, const void* dataSrc, uint32_t rowPitch, uint32_t slicePitch, ID3D12Resource* dest, ID3D12Resource* src)
{
    GfxCommandList& cmdList = context.GetCommandList();

    // store buffer in upload heap
    D3D12_SUBRESOURCE_DATA data = {};
    data.pData = dataSrc;
    data.RowPitch = rowPitch;
    data.SlicePitch = slicePitch;

    // we are now creating a command with the command list to copy the data from the upload heap to the default heap
    const UINT MaxSubresources = 1;
    const UINT64 IntermediateOffset = 0;
    const UINT FirstSubresource = 0;
    const UINT NumSubresources = 1;
    const UINT64 r = UpdateSubresources<MaxSubresources>(cmdList.Dev(), dest, src, IntermediateOffset, FirstSubresource, NumSubresources, &data);
    assert(r);
}

void GfxBufferCommon::InitializeBufferWithInitData(GfxContext& context, const void* initData)
{
    // Create heap to hold final buffer data
    CreateHeap(context, D3D12_HEAP_TYPE_DEFAULT, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), D3D12_RESOURCE_STATE_COPY_DEST, m_D3D12MABufferAllocation);
    assert(m_D3D12MABufferAllocation);

    // create upload heap to hold upload init data, and perform a CopyBufferRegion
    D3D12MA::Allocation* uploadHeapAlloc = nullptr;
    CreateHeap(context, D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), D3D12_RESOURCE_STATE_GENERIC_READ, uploadHeapAlloc);
    UploadInitData(context, initData, m_SizeInBytes, m_SizeInBytes, m_D3D12MABufferAllocation->GetResource(), uploadHeapAlloc->GetResource());

    // we don't need this upload heap anymore... release it next frame
    g_GfxManager.AddGraphicCommand([uploadHeapAlloc]()
        {
            uploadHeapAlloc->GetResource()->Release();
            uploadHeapAlloc->Release();
        });
}

D3D12MA::Allocation* GfxVertexBuffer::Initialize(GfxContext& context, const void* vList, uint32_t numVertices, uint32_t vertexSize)
{
    bbeProfileFunction();

    assert(vList);
    assert(numVertices > 0);
    assert(vertexSize > 0);

    m_StrideInBytes = vertexSize;
    m_SizeInBytes = vertexSize * numVertices;

    // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
    SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);

    InitializeBufferWithInitData(context, vList);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    // after uploading init values, transition the vertex buffer data from copy destination state to vertex buffer state
    Transition(context.GetCommandList(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    return m_D3D12MABufferAllocation;
}

D3D12MA::Allocation* GfxIndexBuffer::Initialize(GfxContext& context, const void* iList, uint32_t numIndices, uint32_t indexSize)
{
    bbeProfileFunction();

    assert(iList);
    assert(numIndices > 0);
    assert(indexSize == 2 || indexSize == 4);

    m_Format = indexSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    m_SizeInBytes = numIndices * indexSize;

    // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
    SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);

    InitializeBufferWithInitData(context, iList);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    // after uploading init values, transition the index buffer data from copy destination state to index buffer state
    Transition(context.GetCommandList(), D3D12_RESOURCE_STATE_INDEX_BUFFER);

    return m_D3D12MABufferAllocation;
}

D3D12MA::Allocation* GfxConstantBuffer::Initialize()
{
    bbeProfileFunction();

    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    D3D12MA::ALLOCATION_DESC constantBufferUploadAllocDesc = {};
    constantBufferUploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;


    assert(m_D3D12MABufferAllocation);
    return m_D3D12MABufferAllocation;
}
