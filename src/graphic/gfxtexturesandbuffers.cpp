#include "graphic/gfxtexturesandbuffers.h"

#include "extern/D3D12MemoryAllocator/src/D3D12MemAlloc.h"

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

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();

    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    m_Resource = resource;
    m_Resource->SetName(L"Render Target View");

    {
        bbeProfile("CreateRenderTargetView");
        gfxDevice.Dev()->CreateRenderTargetView(m_Resource.Get(), nullptr, m_GfxDescriptorHeap.GetCPUDescHandle());
    }

    m_Format = format;
}

GfxVertexBuffer::~GfxVertexBuffer()
{
    bbeProfileFunction();

    if (m_D3D12MABufferAllocation)
    {
        m_D3D12MABufferAllocation->Release();
        m_D3D12MABufferAllocation = nullptr;
    }
}

void GfxVertexBuffer::Initialize(GfxContext& context, const void* vertexData, uint32_t numVertices, uint32_t vertexSize)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = GfxManager::GetInstance().GetGfxDevice();
    D3D12MA::Allocator* d3d12MemoryAllocator = gfxDevice.GetD3D12MemoryAllocator();

    // Default heap is memory on the GPU. Only the GPU has access to this memory
    // To get data into this heap, we will have to upload the data using an upload heap
    D3D12MA::ALLOCATION_DESC vertexBufferAllocDesc = {};
    vertexBufferAllocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    CD3DX12_RESOURCE_DESC vertexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexSize);

    ID3D12Resource* vertexBufferPtr = nullptr;
    DX12_CALL(d3d12MemoryAllocator->CreateResource(
        &vertexBufferAllocDesc,
        &vertexBufferResourceDesc,      // resource description for a buffer
        D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
        nullptr,                        // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
        &m_D3D12MABufferAllocation,
        IID_PPV_ARGS(&vertexBufferPtr)));
    SetCurrentState(D3D12_RESOURCE_STATE_COPY_DEST);

    m_Resource = vertexBufferPtr;
    m_Resource->SetName(L"Vertex Buffer Resource Heap");

    // Upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
    // We will upload the vertex buffer using this heap to the default heap
    D3D12MA::ALLOCATION_DESC vBufferUploadAllocDesc = {};
    vBufferUploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    CD3DX12_RESOURCE_DESC vertexBufferUploadResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexSize);

    ID3D12Resource* vBufferUploadHeap = nullptr;
    D3D12MA::Allocation* vBufferUploadHeapAllocation = nullptr;
    DX12_CALL(d3d12MemoryAllocator->CreateResource(
        &vBufferUploadAllocDesc,
        &vertexBufferUploadResourceDesc,   // resource description for a buffer
        D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
        nullptr,
        &vBufferUploadHeapAllocation,
        IID_PPV_ARGS(&vBufferUploadHeap)));
    vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

    // create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
    m_VertexBufferView.BufferLocation = m_Resource->GetGPUVirtualAddress();
    m_VertexBufferView.StrideInBytes = vertexSize;
    m_VertexBufferView.SizeInBytes = vertexSize * numVertices;

    // store vertex buffer in upload heap
    D3D12_SUBRESOURCE_DATA vertexSubresourceData = {};
    vertexSubresourceData.pData = reinterpret_cast<const void*>(vertexData); // pointer to our vertex array
    vertexSubresourceData.RowPitch = vertexSize;                             // size of all our triangle vertex data
    vertexSubresourceData.SlicePitch = vertexSize;                           // also the size of our triangle vertex data

    GfxCommandList& cmdList = context.GetCommandList();

    // we are now creating a command with the command list to copy the data from
    // the upload heap to the default heap
    const UINT64 IntermediateOffset = 0;
    const UINT FirstSubresource = 0;
    const UINT NumSubresources = 1;
    const UINT64 r = UpdateSubresources(cmdList.Dev(), m_Resource.Get(), vBufferUploadHeap, IntermediateOffset, FirstSubresource, NumSubresources, &vertexSubresourceData);
    assert(r);

    // transition the vertex buffer data from copy destination state to vertex buffer state
    Transition(cmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // release the upload heap in the beginning of the next gfx frame, when the upload should be complete
    GfxManager::GetInstance().AddGraphicCommand([&, vBufferUploadHeap, vBufferUploadHeapAllocation]()
        {
            vBufferUploadHeapAllocation->Release();
            vBufferUploadHeap->Release();
        });
}
