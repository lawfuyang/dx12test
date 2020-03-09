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

// TODO: this function does not seem generic enough for future texture uses...
void GfxTexture::Initialize(DXGI_FORMAT format, const std::string& resourceName)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    m_Resource->SetName(resourceName == "" ? L"GfxTexture" : utf8_decode(resourceName).c_str());

    gfxDevice.Dev()->CreateRenderTargetView(m_Resource.Get(), nullptr, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());

    m_Format = format;
}

void GfxTexture::InitializeForSwapChain(ComPtr<IDXGISwapChain4>& swapChain, DXGI_FORMAT format, uint32_t backBufferIdx)
{
    bbeProfileFunction();
    assert(!m_Resource);
    assert(swapChain);
    assert(format != DXGI_FORMAT_UNKNOWN);
    assert(backBufferIdx <= 3); // you dont need more than 3 back buffers...

    DX12_CALL(swapChain->GetBuffer(backBufferIdx, IID_PPV_ARGS(&m_Resource)));
    Initialize(format, StringFormat("Back Buffer RTV %d", backBufferIdx));
}

GfxBufferCommon::~GfxBufferCommon()
{
    assert(m_D3D12MABufferAllocation == nullptr);
}

void GfxBufferCommon::Release()
{
    bbeProfileFunction();

    if (m_D3D12MABufferAllocation)
    {
        g_Log.info("Destroying D3D12MA::Allocation '{}'", utf8_encode(m_D3D12MABufferAllocation->GetName()));

        m_D3D12MABufferAllocation->Release();
        m_D3D12MABufferAllocation = nullptr;
    }
}

D3D12MA::Allocation* GfxBufferCommon::CreateHeap(GfxContext& context, D3D12_HEAP_TYPE heapType, const CD3DX12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState)
{
    GfxDevice& gfxDevice = context.GetDevice();
    D3D12MA::Allocator& d3d12MemoryAllocator = gfxDevice.GetD3D12MemoryAllocator();

    D3D12MA::ALLOCATION_DESC vertexBufferAllocDesc = {};
    vertexBufferAllocDesc.HeapType = heapType;

    D3D12MA::Allocation* allocHandle = nullptr;
    ID3D12Resource* newHeap = nullptr;
    DX12_CALL(d3d12MemoryAllocator.CreateResource(
        &vertexBufferAllocDesc,
        &resourceDesc,
        initialState,
        nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
        &allocHandle,
        IID_PPV_ARGS(&newHeap)));

    assert(allocHandle);
    assert(newHeap);

    return allocHandle;
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

void GfxBufferCommon::SetInternalAllocName(const std::string& name)
{
    const std::wstring resourceName = name.length() ? utf8_decode(name) : L"GfxConstantBuffer";
    m_D3D12MABufferAllocation->SetName(reinterpret_cast<LPCWSTR>(resourceName.data()));
    m_D3D12MABufferAllocation->GetResource()->SetName(reinterpret_cast<LPCWSTR>(resourceName.data()));
}

void GfxBufferCommon::InitializeBufferWithInitData(GfxContext& context, const void* initData)
{
    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = CreateHeap(context, D3D12_HEAP_TYPE_DEFAULT, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), D3D12_RESOURCE_STATE_COPY_DEST);
    assert(m_D3D12MABufferAllocation);

    // create upload heap to hold upload init data
    D3D12MA::Allocation* uploadHeapAlloc = CreateHeap(context, D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), D3D12_RESOURCE_STATE_GENERIC_READ);

    // upload init data via CopyBufferRegion
    UploadInitData(context, initData, m_SizeInBytes, m_SizeInBytes, m_D3D12MABufferAllocation->GetResource(), uploadHeapAlloc->GetResource());

    // we don't need this upload heap anymore... release it next frame
    g_GfxManager.AddGraphicCommand([uploadHeapAlloc]()
        {
            uploadHeapAlloc->GetResource()->Release();
            uploadHeapAlloc->Release();
        });
}

void GfxVertexBuffer::Initialize(GfxContext& context, const void* vList, uint32_t numVertices, uint32_t vertexSize, const std::string& resourceName)
{
    bbeProfileFunction();
    assert(!m_Resource);
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

    SetInternalAllocName(resourceName);
}

void GfxIndexBuffer::Initialize(GfxContext& context, const void* iList, uint32_t numIndices, uint32_t indexSize, const std::string& resourceName)
{
    bbeProfileFunction();
    assert(!m_Resource);
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

    SetInternalAllocName(resourceName);
}

void GfxConstantBuffer::Initialize(GfxContext& context, uint32_t bufferSize, const std::string& resourceName)
{
    bbeProfileFunction();
    assert((bbeIsAligned(bufferSize, 16))); // CBuffers are 16 bytes aligned

    GfxDevice& gfxDevice = context.GetDevice();

    m_SizeInBytes = bbeAlignVal(bufferSize, 256); // CB size is required to be 256-byte aligned.

    // init desc heap for cbuffer
    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    // Create upload heap for constant buffer
    m_D3D12MABufferAllocation = CreateHeap(context, D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), D3D12_RESOURCE_STATE_GENERIC_READ);
    assert(m_D3D12MABufferAllocation);

    // Describe and create a constant buffer view.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_D3D12MABufferAllocation->GetResource()->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_SizeInBytes;
    gfxDevice.Dev()->CreateConstantBufferView(&cbvDesc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());

    // Map and initialize the constant buffer. We don't unmap this until the app closes. 
    // Keeping things mapped for the lifetime of the resource is okay.
    CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
    DX12_CALL(m_D3D12MABufferAllocation->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&m_CBufferMemory)));

    // zero-init memory for constant buffer
    memset(m_CBufferMemory, 0, m_SizeInBytes);

    SetInternalAllocName(resourceName);
}

void GfxConstantBuffer::Update(const void* data) const
{
    assert(data);
    assert(m_CBufferMemory);

    memcpy(m_CBufferMemory, data, m_SizeInBytes);
}
