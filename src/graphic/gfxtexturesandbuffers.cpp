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

D3D12MA::Allocation* GfxBufferCommon::CreateHeap(GfxContext& context, D3D12_HEAP_TYPE heapType, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState)
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

void GfxBufferCommon::InitializeBufferWithInitData(GfxContext& context, uint32_t uploadBufferSize, uint32_t row, uint32_t pitch, const void* initData)
{
    // create upload heap to hold upload init data
    D3D12MA::Allocation* uploadHeapAlloc = CreateHeap(context, D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ);

    // upload init data via CopyBufferRegion
    UploadInitData(context, initData, row, pitch, m_D3D12MABufferAllocation->GetResource(), uploadHeapAlloc->GetResource());

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
    SetHazardTrackedState(D3D12_RESOURCE_STATE_COPY_DEST);

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = CreateHeap(context, D3D12_HEAP_TYPE_DEFAULT, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), D3D12_RESOURCE_STATE_COPY_DEST);
    assert(m_D3D12MABufferAllocation);

    InitializeBufferWithInitData(context, m_SizeInBytes, m_SizeInBytes, m_SizeInBytes, vList);
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
    SetHazardTrackedState(D3D12_RESOURCE_STATE_COPY_DEST);

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = CreateHeap(context, D3D12_HEAP_TYPE_DEFAULT, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), D3D12_RESOURCE_STATE_COPY_DEST);
    assert(m_D3D12MABufferAllocation);

    InitializeBufferWithInitData(context, m_SizeInBytes, m_SizeInBytes, m_SizeInBytes, iList);
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

void GfxTexture::Initialize(GfxContext& context, 
                            DXGI_FORMAT format,
                            uint32_t width, 
                            uint32_t height, 
                            D3D12_RESOURCE_FLAGS flags,
                            const void* initData, 
                            const std::string& resourceName)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    m_SizeInBytes = bbeRoundToNextMultiple(width * height * GetBitsPerPixel(format), 8) / 8;
    m_Format = format;

    // init desc heap for texture
    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Flags = flags;

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = CreateHeap(context, D3D12_HEAP_TYPE_DEFAULT, desc, initData ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_COMMON);
    assert(m_D3D12MABufferAllocation);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    // if init data is specified, upload it
    if (initData)
    {
        // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
        SetHazardTrackedState(D3D12_RESOURCE_STATE_COPY_DEST);

        const uint32_t uploadBufferSize = (uint32_t)GetRequiredIntermediateSize(m_Resource.Get(), 0, 1);
        const uint32_t rowPitch = width * 4;// GetBitsPerPixel(format) / 8;
        InitializeBufferWithInitData(context, uploadBufferSize, rowPitch, rowPitch * height, initData);

        // after uploading init values, transition the texture from copy destination state to common state
        Transition(context.GetCommandList(), D3D12_RESOURCE_STATE_COMMON);
    }

    // Describe and create a SRV for the texture
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    gfxDevice.Dev()->CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());

    SetInternalAllocName(resourceName);
}

void GfxTexture::InitializeForSwapChain(ComPtr<IDXGISwapChain4>& swapChain, DXGI_FORMAT format, uint32_t backBufferIdx)
{
    bbeProfileFunction();

    assert(!m_Resource);
    assert(swapChain);
    assert(format != DXGI_FORMAT_UNKNOWN);
    assert(backBufferIdx <= 3); // you dont need more than 3 back buffers...

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    m_Format = format;

    DX12_CALL(swapChain->GetBuffer(backBufferIdx, IID_PPV_ARGS(&m_Resource)));

    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    m_Resource->SetName(utf8_decode(StringFormat("Back Buffer RTV %d", backBufferIdx)).c_str());

    gfxDevice.Dev()->CreateRenderTargetView(m_Resource.Get(), nullptr, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}
