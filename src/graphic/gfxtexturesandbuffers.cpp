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
        g_Log.info("Destroying D3D12MA::Allocation '{}'", MakeStrFromWStr(m_D3D12MABufferAllocation->GetName()));

        m_D3D12MABufferAllocation->Release();
        m_D3D12MABufferAllocation = nullptr;
    }
}

D3D12MA::Allocation* GfxBufferCommon::CreateHeap(GfxContext& context, D3D12_HEAP_TYPE heapType, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState, const char* resourceName)
{
    bbeProfileFunction();

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

    const std::wstring resourceNameW = MakeWStrFromStr(resourceName);
    allocHandle->SetName(resourceNameW.data());
    allocHandle->GetResource()->SetName(resourceNameW.data());

    g_Log.info("Created D3D12MA::Allocation '{}'", resourceName);

    return allocHandle;
}

void GfxBufferCommon::UploadInitData(GfxContext& context, const void* dataSrc, uint32_t rowPitch, uint32_t slicePitch, ID3D12Resource* dest, ID3D12Resource* src)
{
    bbeProfileFunction();

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

void GfxBufferCommon::InitializeBufferWithInitData(GfxContext& context, uint32_t uploadBufferSize, uint32_t row, uint32_t pitch, const void* initData, const char* resourceName)
{
    bbeProfileFunction();

    // create upload heap to hold upload init data
    char nameBuffer[256] = {};
    strcpy(nameBuffer, resourceName);
    strcat(nameBuffer, " Upload Heap");
    D3D12MA::Allocation* uploadHeapAlloc = CreateHeap(context, D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nameBuffer);

    // upload init data via CopyBufferRegion
    UploadInitData(context, initData, row, pitch, m_D3D12MABufferAllocation->GetResource(), uploadHeapAlloc->GetResource());

    // we don't need this upload heap anymore... release it next frame
    g_GfxManager.AddGraphicCommand([uploadHeapAlloc]()
        {
            uploadHeapAlloc->GetResource()->Release();
            uploadHeapAlloc->Release();
        });
}

void GfxVertexBuffer::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    assert(!m_Resource);

    assert(initParams.m_NumVertices > 0);
    assert(initParams.m_VertexSize > 0);

    m_NumVertices = initParams.m_NumVertices;
    m_StrideInBytes = initParams.m_VertexSize;
    m_SizeInBytes = initParams.m_VertexSize * initParams.m_NumVertices;

    // identify resource initial state
    D3D12_RESOURCE_STATES initialState;
    switch (initParams.m_HeapType)
    {
    case D3D12_HEAP_TYPE_DEFAULT: initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; break;
    case D3D12_HEAP_TYPE_UPLOAD: initialState = D3D12_RESOURCE_STATE_GENERIC_READ; break;
    case D3D12_HEAP_TYPE_READBACK: initialState = D3D12_RESOURCE_STATE_COPY_DEST; break;
    default: assert(0);
    }
    SetHazardTrackedState(initialState);

    const bool hasInitData = initParams.m_InitData != nullptr;

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = CreateHeap(initContext, initParams.m_HeapType, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initialState, initParams.m_ResourceName);
    assert(m_D3D12MABufferAllocation);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    if (hasInitData)
    {
        InitializeBufferWithInitData(initContext, m_SizeInBytes, m_SizeInBytes, m_SizeInBytes, initParams.m_InitData, initParams.m_ResourceName);

        // after uploading init values, transition the vertex buffer data from copy destination state to vertex buffer state
        Transition(initContext.GetCommandList(), initialState);
    }
}

void GfxVertexBuffer::Initialize(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName);

    Initialize(initContext, initParams);
}

void GfxIndexBuffer::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    assert(!m_Resource);
    assert(initParams.m_NumIndices);
    assert(initParams.m_IndexSize == 2 || initParams.m_IndexSize == 4);

    m_NumIndices = initParams.m_NumIndices;
    m_Format = initParams.m_IndexSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    m_SizeInBytes = initParams.m_NumIndices * initParams.m_IndexSize;

    // identify resource initial state
    D3D12_RESOURCE_STATES initialState;
    switch (initParams.m_HeapType)
    {
    case D3D12_HEAP_TYPE_DEFAULT: initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; break;
    case D3D12_HEAP_TYPE_UPLOAD: initialState = D3D12_RESOURCE_STATE_GENERIC_READ; break;
    case D3D12_HEAP_TYPE_READBACK: initialState = D3D12_RESOURCE_STATE_COPY_DEST; break;
    default: assert(0);
    }
    SetHazardTrackedState(initialState);

    const bool hasInitData = initParams.m_InitData != nullptr;

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = CreateHeap(initContext, initParams.m_HeapType, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initialState, initParams.m_ResourceName);
    assert(m_D3D12MABufferAllocation);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    if (hasInitData)
    {
        InitializeBufferWithInitData(initContext, m_SizeInBytes, m_SizeInBytes, m_SizeInBytes, initParams.m_InitData, initParams.m_ResourceName);

        // after uploading init values, transition the index buffer data from copy destination state to index buffer state
        Transition(initContext.GetCommandList(), initialState);
    }
}

void GfxIndexBuffer::Initialize(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName);

    Initialize(initContext, initParams);
}

void GfxConstantBuffer::Initialize(uint32_t bufferSize, const std::string& resourceName)
{
    bbeProfileFunction();
    assert((bbeIsAligned(bufferSize, 16))); // CBuffers are 16 bytes aligned

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, resourceName.c_str());

    m_SizeInBytes = AlignUp(bufferSize, 256); // CB size is required to be 256-byte aligned.

    // init desc heap for cbuffer
    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    // Create upload heap for constant buffer
    m_D3D12MABufferAllocation = CreateHeap(initContext, D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes), D3D12_RESOURCE_STATE_GENERIC_READ, resourceName.c_str());
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
    SIMDMemFill(m_CBufferMemory, bbeVector4::Zero, DivideByMultiple(m_SizeInBytes, 16));
}

void GfxConstantBuffer::Update(const void* data) const
{
    assert(data);
    assert(m_CBufferMemory);

    SIMDMemCopy(m_CBufferMemory, data, DivideByMultiple(m_SizeInBytes, 16));
}

void GfxTexture::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();

    assert(initParams.m_Format != DXGI_FORMAT_UNKNOWN);
    assert(initParams.m_Width > 0);
    assert(initParams.m_Height > 0);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    m_SizeInBytes = initParams.m_Width * initParams.m_Height * GetBytesPerPixel(initParams.m_Format);
    m_Format = initParams.m_Format;

    // init desc heap for texture
    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = initParams.m_Width;
    desc.Height = initParams.m_Height;
    desc.DepthOrArraySize = 1;
    desc.Format = initParams.m_Format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Flags = initParams.m_Flags;

    const bool hasInitData = initParams.m_InitData != nullptr;

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = CreateHeap(initContext, D3D12_HEAP_TYPE_DEFAULT, desc, hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initParams.m_InitialState, initParams.m_ResourceName);
    assert(m_D3D12MABufferAllocation);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    // if init data is specified, upload it
    if (hasInitData)
    {
        // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
        SetHazardTrackedState(D3D12_RESOURCE_STATE_COPY_DEST);

        const uint32_t uploadBufferSize = (uint32_t)GetRequiredIntermediateSize(m_Resource.Get(), 0, 1);
        const uint32_t rowPitch = initParams.m_Width * (GetBitsPerPixel(initParams.m_Format) / 8);
        InitializeBufferWithInitData(initContext, uploadBufferSize, rowPitch, rowPitch * initParams.m_Height, initParams.m_InitData, initParams.m_ResourceName);

        // after uploading init values, transition the texture from copy destination state to common state
        Transition(initContext.GetCommandList(), initParams.m_InitialState);
    }

    // Describe and create a SRV for the texture
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = initParams.m_Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    gfxDevice.Dev()->CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}

void GfxTexture::Initialize(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName);

    Initialize(initContext, initParams);
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

    m_Resource->SetName(MakeWStrFromStr(StringFormat("Back Buffer RTV %d", backBufferIdx)).c_str());

    gfxDevice.Dev()->CreateRenderTargetView(m_Resource.Get(), nullptr, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}
