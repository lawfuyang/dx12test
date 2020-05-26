#include <graphic/gfxtexturesandbuffers.h>

#include <graphic/gfxmanager.h>
#include <graphic/gfxdevice.h>
#include <graphic/gfxcontext.h>
#include <graphic/dx12utils.h>

struct ID3D12ResourceMemoryLayout
{
    uint32_t m_TotalSizeInBytes = 0;
    uint32_t m_RowPitch = 0;
    uint32_t m_SlicePitch = 0;
    uint32_t m_Alignment = 0;
};

static ID3D12ResourceMemoryLayout GetMemoryLayout(const D3D12_RESOURCE_DESC& desc)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    assert(desc.MipLevels > 0);

    const uint32_t arraySize = (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D ? desc.DepthOrArraySize : 1);
    const uint32_t subResourceCount = desc.MipLevels * arraySize;

    const uint32_t ArraySize = 32;

    if (subResourceCount > ArraySize)
        g_Log.warn("subResourceCount = {}. Increase array sizes in {}", subResourceCount, __FUNCTION__);

    InplaceArray<D3D12_PLACED_SUBRESOURCE_FOOTPRINT, ArraySize> layouts(subResourceCount);
    InplaceArray<UINT, ArraySize> numRows(subResourceCount);
    InplaceArray<UINT64, ArraySize> rowSizeInBytes(subResourceCount);
    UINT64 requiredSize = 0;

    gfxDevice.Dev()->GetCopyableFootprints(&desc, 0, subResourceCount, 0, layouts.data(), numRows.data(), rowSizeInBytes.data(), &requiredSize);

    const D3D12_RESOURCE_ALLOCATION_INFO allocInfo = gfxDevice.Dev()->GetResourceAllocationInfo(0, 1, &desc);

    ID3D12ResourceMemoryLayout memoryLayout;
    memoryLayout.m_RowPitch = layouts[0].Footprint.RowPitch;
    memoryLayout.m_SlicePitch = memoryLayout.m_RowPitch * desc.Height;
    memoryLayout.m_Alignment = static_cast<uint32_t>(allocInfo.Alignment);
    memoryLayout.m_TotalSizeInBytes = static_cast<uint32_t>(allocInfo.SizeInBytes);

    return memoryLayout;
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
        ReleaseAllocation(m_D3D12MABufferAllocation);
    }
}

void GfxBufferCommon::ReleaseAllocation(D3D12MA::Allocation*& allocation)
{
    bbeProfileFunction();

    g_Log.info("Destroying D3D12MA::Allocation '{}'", MakeStrFromWStr(allocation->GetName()));

    allocation->GetResource()->Release();
    allocation->Release();
    allocation = nullptr;
}

D3D12MA::Allocation* GfxBufferCommon::CreateHeap(GfxContext& context, const GfxBufferCommon::HeapDesc& heapDesc)
{
    bbeProfileFunction();

    GfxDevice& gfxDevice = context.GetDevice();
    D3D12MA::Allocator& d3d12MemoryAllocator = gfxDevice.GetD3D12MemoryAllocator();

    D3D12MA::ALLOCATION_DESC vertexBufferAllocDesc = {};
    vertexBufferAllocDesc.HeapType = heapDesc.m_HeapType;

    D3D12MA::Allocation* allocHandle = nullptr;
    ID3D12Resource* newHeap = nullptr;
    DX12_CALL(d3d12MemoryAllocator.CreateResource(
        &vertexBufferAllocDesc,
        &heapDesc.m_ResourceDesc,
        heapDesc.m_InitialState,
        heapDesc.m_ClearValue.Format == DXGI_FORMAT_UNKNOWN ? nullptr : &heapDesc.m_ClearValue,
        &allocHandle,
        IID_PPV_ARGS(&newHeap)));

    assert(allocHandle);
    assert(newHeap);

    const StaticWString<256> resourceNameW = MakeWStrFromStr(heapDesc.m_ResourceName).c_str();
    allocHandle->SetName(resourceNameW.c_str());
    allocHandle->GetResource()->SetName(resourceNameW.c_str());

    StaticString<256> heapName = heapDesc.m_ResourceName;
    heapName += " Heap";
    SetD3DDebugName(newHeap, heapName.c_str());

    g_Log.info("Created D3D12MA::Allocation '{}'", heapDesc.m_ResourceName);

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

void GfxBufferCommon::InitializeBufferWithInitData(GfxContext& context, uint32_t uploadBufferSize, uint32_t rowPitch, uint32_t slicePitch, const void* initData, const char* resourceName)
{
    bbeProfileFunction();

    // create upload heap to hold upload init data
    StaticString<256> nameBuffer = resourceName;
    nameBuffer += " Upload Buffer";

    GfxBufferCommon::HeapDesc heapDesc;
    heapDesc.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
    heapDesc.m_ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    heapDesc.m_InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    heapDesc.m_ResourceName = nameBuffer.c_str();

    D3D12MA::Allocation* uploadHeapAlloc = CreateHeap(context, heapDesc);

    // upload init data via CopyBufferRegion
    UploadInitData(context, initData, rowPitch, slicePitch, m_D3D12MABufferAllocation->GetResource(), uploadHeapAlloc->GetResource());

    // we don't need this upload heap anymore... release it next frame
    g_GfxManager.AddGraphicCommand([uploadHeapAlloc]()
        {
            D3D12MA::Allocation* copy = uploadHeapAlloc;
            GfxBufferCommon::ReleaseAllocation(copy);
        });
}

void GfxVertexBuffer::Initialize(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName);

    Initialize(initContext, initParams);

    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());
}

void GfxVertexBuffer::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);
    assert(!m_D3D12MABufferAllocation);

    assert(initParams.m_NumVertices > 0);
    assert(initParams.m_VertexSize > 0);

    m_NumVertices = initParams.m_NumVertices;
    m_StrideInBytes = initParams.m_VertexSize;
    m_SizeInBytes = initParams.m_VertexSize * initParams.m_NumVertices;

    if (m_SizeInBytes > D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM * 1024 * 1024)
        assert(false);

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
    GfxBufferCommon::HeapDesc heapDesc;
    heapDesc.m_HeapType = initParams.m_HeapType;
    heapDesc.m_ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes);
    heapDesc.m_InitialState = hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;
    heapDesc.m_ResourceName = initParams.m_ResourceName.c_str();

    m_D3D12MABufferAllocation = CreateHeap(initContext, heapDesc);
    assert(m_D3D12MABufferAllocation);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    if (hasInitData)
    {
        InitializeBufferWithInitData(initContext, m_SizeInBytes, m_SizeInBytes, m_SizeInBytes, initParams.m_InitData, initParams.m_ResourceName.c_str());

        // after uploading init values, transition the vertex buffer data from copy destination state to vertex buffer state
        //Transition(initContext.GetCommandList(), initialState);
        initContext.TransitionResource(*this, initialState, true);
    }
}

void GfxIndexBuffer::Initialize(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName);

    Initialize(initContext, initParams);

    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());
}

void GfxIndexBuffer::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);

    assert(!m_D3D12MABufferAllocation);
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
    GfxBufferCommon::HeapDesc heapDesc;
    heapDesc.m_HeapType = initParams.m_HeapType;
    heapDesc.m_ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes);
    heapDesc.m_InitialState = hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;
    heapDesc.m_ResourceName = initParams.m_ResourceName.c_str();

    m_D3D12MABufferAllocation = CreateHeap(initContext, heapDesc);
    assert(m_D3D12MABufferAllocation);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    if (hasInitData)
    {
        InitializeBufferWithInitData(initContext, m_SizeInBytes, m_SizeInBytes, m_SizeInBytes, initParams.m_InitData, initParams.m_ResourceName.c_str());

        // after uploading init values, transition the index buffer data from copy destination state to index buffer state
        initContext.TransitionResource(*this, initialState, true);
    }
}

void GfxConstantBuffer::Initialize(uint32_t bufferSize, const std::string& resourceName)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, resourceName.c_str());

    Initialize(initContext, bufferSize, resourceName);

    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());
}

void GfxConstantBuffer::Initialize(GfxContext& initContext, uint32_t bufferSize, const std::string& resourceName)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);

    assert((bbeIsAligned(bufferSize, 16))); // CBuffers are 16 bytes aligned

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    m_SizeInBytes = AlignUp(bufferSize, 256); // CB size is required to be 256-byte aligned.

    // init desc heap for cbuffer
    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

    // Create upload heap for constant buffer
    GfxBufferCommon::HeapDesc heapDesc;
    heapDesc.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;
    heapDesc.m_ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes);
    heapDesc.m_InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    heapDesc.m_ResourceName = resourceName.c_str();

    m_D3D12MABufferAllocation = CreateHeap(initContext, heapDesc);
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

void GfxTexture::Initialize(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName);

    Initialize(initContext, initParams);

    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());
}

void GfxTexture::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);

    assert(!m_D3D12MABufferAllocation);
    assert(initParams.m_Format != DXGI_FORMAT_UNKNOWN);
    assert(initParams.m_Width > 0);
    assert(initParams.m_Height > 0);

    m_Format = initParams.m_Format;

    // init desc heap for texture
    D3D12_DESCRIPTOR_HEAP_TYPE heapType = initParams.m_ViewType == SRV ? D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV : D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags = initParams.m_ViewType == SRV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    m_GfxDescriptorHeap.Initialize(heapType, heapFlags);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = initParams.m_Dimension;
    desc.Width = initParams.m_Width;
    desc.Height = initParams.m_Height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = initParams.m_Format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Flags = initParams.m_Flags;

    const ID3D12ResourceMemoryLayout memoryLayout = GetMemoryLayout(desc);

    m_SizeInBytes = memoryLayout.m_TotalSizeInBytes;

    const bool hasInitData = initParams.m_InitData != nullptr;

    // Create heap to hold final buffer data
    GfxBufferCommon::HeapDesc heapDesc;
    heapDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
    heapDesc.m_ResourceDesc = desc;
    heapDesc.m_InitialState = hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initParams.m_InitialState;
    heapDesc.m_ClearValue = initParams.m_ClearValue;
    heapDesc.m_ResourceName = initParams.m_ResourceName.c_str();

    m_D3D12MABufferAllocation = CreateHeap(initContext, heapDesc);
    assert(m_D3D12MABufferAllocation);
    m_Resource = m_D3D12MABufferAllocation->GetResource();

    // if init data is specified, upload it
    if (hasInitData)
    {
        // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
        SetHazardTrackedState(D3D12_RESOURCE_STATE_COPY_DEST);

        const uint32_t uploadBufferSize = (uint32_t)GetRequiredIntermediateSize(m_Resource.Get(), 0, 1);

        InitializeBufferWithInitData(initContext, uploadBufferSize, memoryLayout.m_RowPitch, uploadBufferSize, initParams.m_InitData, initParams.m_ResourceName.c_str());

        // after uploading init values, transition the texture from copy destination state to common state
        initContext.TransitionResource(*this, initParams.m_InitialState, true);
    }

    if (initParams.m_ViewType == SRV)
    {
        CreateSRV(initParams);
    }
    else
    {
        assert(initParams.m_ViewType == DSV);
        CreateDSV(initParams);
    }
}

void GfxTexture::CreateSRV(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = initParams.m_Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    gfxDevice.Dev()->CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}

void GfxTexture::CreateDSV(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    assert(initParams.m_Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = initParams.m_Format;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    gfxDevice.Dev()->CreateDepthStencilView(m_Resource.Get(), &depthStencilDesc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}

void GfxTexture::InitializeForSwapChain(ComPtr<IDXGISwapChain4>& swapChain, DXGI_FORMAT format, uint32_t backBufferIdx)
{
    bbeProfileFunction();

    assert(!m_D3D12MABufferAllocation);
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
