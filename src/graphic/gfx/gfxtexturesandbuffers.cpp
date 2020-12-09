#include <graphic/gfx/gfxtexturesandbuffers.h>
#include <graphic/pch.h>

#include <system/imguimanager.h>

#define TRACK_GFX_HEAP_ALLOCS
//#define DEBUG_PRINT_GFX_MEMORY_ALLOCS

#if defined(TRACK_GFX_HEAP_ALLOCS) && defined(DEBUG_PRINT_GFX_MEMORY_ALLOCS)
    #define bbeLogGfxAlloc(StrFormat, ...) g_Log.info(StrFormat, __VA_ARGS__);
#else
    #define bbeLogGfxAlloc(...) __noop
#endif

#if defined(TRACK_GFX_HEAP_ALLOCS)
    static ConcurrentUnorderedMap<D3D12MA::Allocation*, uint32_t> gs_HeapAllocs;

    #define TRACK_GFX_ALLOC_REF_COUNT(allocHandle) ++gs_HeapAllocs[allocHandle]
    #define UNTRACK_GFX_ALLOC_REF_COUNT(allocHandle) --gs_HeapAllocs[allocHandle]

    void CheckAllD3D12AllocsReleased()
    {
        uint32_t numLeaks = 0;
        for (const auto& elem : gs_HeapAllocs)
        {
            if (elem.second)
            {
                ++numLeaks;
                g_Log.critical("\nUnreleased D3D12MA Allocation: '{}'\n", StringUtils::WideToUtf8(elem.first->GetName()));
            }
        }
        assert(numLeaks == 0);
    }
#else
    #define TRACK_GFX_ALLOC_REF_COUNT(obj, name) __noop
    #define UNTRACK_GFX_ALLOC_REF_COUNT(obj) __noop
    void CheckAllD3D12AllocsReleased() {}
#endif // #define TRACK_GFX_HEAP_ALLOCS

void GfxBufferCommon::Release()
{
    bbeProfileFunction();

    if (m_D3D12MABufferAllocation)
    {
        ReleaseAllocation(m_D3D12MABufferAllocation);
        m_D3D12MABufferAllocation = nullptr;
    }
}

void GfxBufferCommon::ReleaseAllocation(D3D12MA::Allocation* allocation)
{
    bbeProfileFunction();

    bbeLogGfxAlloc("Destroying D3D12MA::Allocation '{}'", StringUtils::WideToUtf8(allocation->GetName()));

    UNTRACK_GFX_ALLOC_REF_COUNT(allocation);

    allocation->GetResource()->Release();
    allocation->Release();
}

D3D12MA::Allocation* GfxBufferCommon::CreateHeap(const GfxBufferCommon::HeapDesc& heapDesc)
{
    bbeProfileFunction();

    D3D12MA::ALLOCATION_DESC bufferAllocDesc{};
    bufferAllocDesc.HeapType = heapDesc.m_HeapType;

    D3D12MA::Allocation* allocHandle = nullptr;
    ID3D12Resource* newHeap = nullptr;

    DX12_CALL(g_GfxMemoryAllocator.Dev().CreateResource(
        &bufferAllocDesc,
        &heapDesc.m_ResourceDesc,
        heapDesc.m_InitialState,
        heapDesc.m_ClearValue.Format == DXGI_FORMAT_UNKNOWN ? nullptr : &heapDesc.m_ClearValue,
        &allocHandle,
        IID_PPV_ARGS(&newHeap)));

    assert(allocHandle);
    assert(newHeap);

    const StaticWString<128> resourceNameW = StringUtils::Utf8ToWide(heapDesc.m_ResourceName.c_str());
    allocHandle->SetName(resourceNameW.c_str());
    allocHandle->GetResource()->SetName(resourceNameW.c_str());

    StaticString<256> heapName = heapDesc.m_ResourceName;
    heapName += " Heap";
    SetD3DDebugName(allocHandle->GetHeap(), heapName.c_str());
    SetD3DDebugName(newHeap, heapName.c_str());

    TRACK_GFX_ALLOC_REF_COUNT(allocHandle);

    bbeLogGfxAlloc("Created D3D12MA::Allocation '{}'", heapDesc.m_ResourceName);

    return allocHandle;
}

void GfxBufferCommon::UploadInitData(GfxContext& context, const void* dataSrc, uint32_t rowPitch, uint32_t slicePitch, ID3D12Resource* dest, ID3D12Resource* src)
{
    bbeProfileFunction();

    GfxCommandList& cmdList = context.GetCommandList();

    // store buffer in upload heap
    D3D12_SUBRESOURCE_DATA data{};
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

    D3D12MA::Allocation* uploadHeapAlloc = CreateHeap(heapDesc);

    // upload init data via CopyBufferRegion
    UploadInitData(context, initData, rowPitch, slicePitch, m_D3D12MABufferAllocation->GetResource(), uploadHeapAlloc->GetResource());

    // we don't need this upload heap anymore... release it 2 frames later.
    g_GfxManager.AddDoubleDeferredGraphicCommand([uploadHeapAlloc]() { GfxBufferCommon::ReleaseAllocation(uploadHeapAlloc); });
}

void GfxVertexBuffer::Initialize(const InitParams& initParams)
{
    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName.c_str());

    Initialize(initContext, initParams);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    g_GfxCommandListsManager.QueueCommandListToExecute(&initContext.GetCommandList());
}

void GfxVertexBuffer::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);
    assert(!m_D3D12MABufferAllocation);

    assert(initParams.m_NumVertices > 0);
    assert(initParams.m_VertexSize > 0);

    m_InitParams = initParams;
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
    m_CurrentResourceState = initialState;

    const bool hasInitData = initParams.m_InitData != nullptr;

    // Create heap to hold final buffer data
    GfxBufferCommon::HeapDesc heapDesc;
    heapDesc.m_HeapType = initParams.m_HeapType;
    heapDesc.m_ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes);
    heapDesc.m_InitialState = hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;
    heapDesc.m_ResourceName = initParams.m_ResourceName.c_str();

    m_D3D12MABufferAllocation = CreateHeap(heapDesc);
    assert(m_D3D12MABufferAllocation);
    m_HazardTrackedResource = m_D3D12MABufferAllocation->GetResource();

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
    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName.c_str());

    Initialize(initContext, initParams);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    g_GfxCommandListsManager.QueueCommandListToExecute(&initContext.GetCommandList());
}

void GfxIndexBuffer::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);

    assert(!m_D3D12MABufferAllocation);
    assert(initParams.m_NumIndices);
    assert(initParams.m_IndexSize == 2 || initParams.m_IndexSize == 4);

    m_InitParams = initParams;
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
    m_CurrentResourceState = initialState;

    const bool hasInitData = initParams.m_InitData != nullptr;

    // Create heap to hold final buffer data
    GfxBufferCommon::HeapDesc heapDesc;
    heapDesc.m_HeapType = initParams.m_HeapType;
    heapDesc.m_ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_SizeInBytes);
    heapDesc.m_InitialState = hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initialState;
    heapDesc.m_ResourceName = initParams.m_ResourceName.c_str();

    m_D3D12MABufferAllocation = CreateHeap(heapDesc);
    assert(m_D3D12MABufferAllocation);
    m_HazardTrackedResource = m_D3D12MABufferAllocation->GetResource();

    if (hasInitData)
    {
        InitializeBufferWithInitData(initContext, m_SizeInBytes, m_SizeInBytes, m_SizeInBytes, initParams.m_InitData, initParams.m_ResourceName.c_str());

        // after uploading init values, transition the index buffer data from copy destination state to index buffer state
        initContext.TransitionResource(*this, initialState, true);
    }
}

ForwardDeclareSerializerFunctions(GfxTexture);

static D3D12_DESCRIPTOR_HEAP_TYPE GfxTextureViewTypeToHeapType(GfxTexture::ViewType viewType)
{
    switch (viewType)
    {
    case GfxTexture::SRV:
    case GfxTexture::UAV:
        return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    case GfxTexture::DSV: return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    case GfxTexture::RTV: return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    }

    assert(false);
    return (D3D12_DESCRIPTOR_HEAP_TYPE)0xDEADBEEF;
}

D3D12_RESOURCE_DESC GfxTexture::GetDescForGfxTexture(const GfxTexture::InitParams& i)
{
    // TODO: D3D12_RESOURCE_DIMENSION_TEXTURE1D, D3D12_RESOURCE_DIMENSION_TEXTURE3D
    switch (i.m_Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
    {
        assert(i.m_Format != DXGI_FORMAT_UNKNOWN);
        assert(i.m_TexParams.m_Width > 0 && i.m_TexParams.m_Width <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
        assert(i.m_TexParams.m_Height > 0 && i.m_TexParams.m_Height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);

        // TODO: Texture2DArray, Mips
        const uint32_t ArraySize = 1;
        const uint32_t MipLevels = 1;
        return CD3DX12_RESOURCE_DESC::Tex2D(i.m_Format, i.m_TexParams.m_Width, i.m_TexParams.m_Height, ArraySize, MipLevels, 1, 0, i.m_Flags);
    }
    case D3D12_RESOURCE_DIMENSION_BUFFER:
    {
        const uint32_t width = i.m_BufferParams.m_NumElements * i.m_BufferParams.m_StructureByteStride;
        static const float s_SizeLimitMb = [] 
        {
            D3D12MA::Budget gpuBudget{};
            g_GfxMemoryAllocator.Dev().GetBudget(&gpuBudget, nullptr);

            static const float A_TERM = 128.0f;
            static const float B_TERM = 0.25f;
            static const float C_TERM = 2048.0f;
            static_assert(A_TERM == D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM);
            static_assert(B_TERM == D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_B_TERM);
            static_assert(C_TERM == D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM);

            return std::min(std::max(A_TERM, B_TERM * BBE_MB(gpuBudget.BudgetBytes)), C_TERM);
        }();

        assert(i.m_Format == DXGI_FORMAT_UNKNOWN);
        assert(BBE_TO_MB(width) < s_SizeLimitMb);

        return CD3DX12_RESOURCE_DESC::Buffer(width, i.m_Flags);
    }
    }

    assert(false);
    return D3D12_RESOURCE_DESC{};
}

void GfxTexture::Initialize(const InitParams& initParams)
{
    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName.c_str());
    Initialize(initContext, initParams);
}

void GfxTexture::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);

    assert(!m_D3D12MABufferAllocation);

    m_InitParams = initParams;

    // init desc heap for texture
    m_GfxDescriptorHeap.Initialize(GfxTextureViewTypeToHeapType(initParams.m_ViewType), D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

    const D3D12_RESOURCE_DESC desc = GetDescForGfxTexture(initParams);

    UINT64 rowSizeInBytes = 0;
    {
        UINT FirstSubresource = 0;
        UINT NumSubresources = 1;
        const UINT64 BaseOffset = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts{};
        UINT numRows = 0;
        UINT64 totalBytes = 0;
        g_GfxManager.GetGfxDevice().Dev()->GetCopyableFootprints(&desc, FirstSubresource, NumSubresources, BaseOffset, &layouts, &numRows, &rowSizeInBytes, &totalBytes);

        m_SizeInBytes = (uint32_t)totalBytes;
    }

    const bool hasInitData = initParams.m_InitData != nullptr;

    // Create heap to hold final buffer data
    GfxBufferCommon::HeapDesc heapDesc;
    heapDesc.m_HeapType = D3D12_HEAP_TYPE_DEFAULT;
    heapDesc.m_ResourceDesc = desc;
    heapDesc.m_InitialState = hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initParams.m_InitialState;
    heapDesc.m_ClearValue = initParams.m_ClearValue;
    heapDesc.m_ResourceName = initParams.m_ResourceName.c_str();

    m_D3D12MABufferAllocation = CreateHeap(heapDesc);
    assert(m_D3D12MABufferAllocation);
    m_HazardTrackedResource = m_D3D12MABufferAllocation->GetResource();

    // if init data is specified, upload it
    if (hasInitData)
    {
        // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
        m_CurrentResourceState = D3D12_RESOURCE_STATE_COPY_DEST;

        InitializeBufferWithInitData(initContext, m_SizeInBytes, (uint32_t)rowSizeInBytes, (uint32_t)(rowSizeInBytes * desc.Height), initParams.m_InitData, initParams.m_ResourceName.c_str());

        // after uploading init values, transition the texture from copy destination state to common state
        initContext.TransitionResource(*this, initParams.m_InitialState, true);
    }

    switch (initParams.m_ViewType)
    {
    case RTV: CreateRTV(initParams); break;
    case DSV: CreateDSV(initParams); break;
    case SRV: CreateSRV(initParams); break;
    case UAV: CreateUAV(initParams); break;
    }

    g_GfxCommandListsManager.QueueCommandListToExecute(&initContext.GetCommandList());
}

void GfxTexture::CreateDSV(const InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    assert(initParams.m_Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc{};
    depthStencilDesc.Format = initParams.m_Format;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    gfxDevice.Dev()->CreateDepthStencilView(GetD3D12Resource(), &depthStencilDesc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}

void GfxTexture::UpdateIMGUI()
{
    ScopedIMGUIID scopedID{ this };

    ID3D12Resource* gfxResource = m_D3D12MABufferAllocation->GetResource();
    D3D12_RESOURCE_DESC desc = gfxResource->GetDesc();

    StaticString<256> nameBuffer = GetD3DDebugName(gfxResource);
    ImGui::InputText("Name", nameBuffer.data(), nameBuffer.capacity(), ImGuiInputTextFlags_ReadOnly);

    const bbeVector2U dims{ (uint32_t)desc.Width, desc.Height };
    ImGui::InputInt2("Dimensions", (int*)&dims, ImGuiInputTextFlags_ReadOnly);
}

void GfxTexture::CreateRTV(const InitParams& initParams)
{
    assert(initParams.m_Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D);

    CD3D12_RENDER_TARGET_VIEW_DESC desc{ D3D12_RTV_DIMENSION_TEXTURE2D, initParams.m_Format };
    g_GfxManager.GetGfxDevice().Dev()->CreateRenderTargetView(GetD3D12Resource(), &desc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}

void GfxTexture::CreateSRV(const InitParams& initParams)
{
    // TODO: Mips

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
    SRVDesc.Format = initParams.m_Format;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    const uint32_t MipLevels = 1;

    // TODO: Texture2DArray
    //if (m_ArraySize > 1)
    //{
    //    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    //    SRVDesc.Texture2DArray.MipLevels = MipLevels;
    //    SRVDesc.Texture2DArray.MostDetailedMip = 0;
    //    SRVDesc.Texture2DArray.FirstArraySlice = 0;
    //    SRVDesc.Texture2DArray.ArraySize = (UINT)m_ArraySize;
    //}

    // TODO: MSAA SRV
    //else if (m_FragmentCount > 1)
    //{
    //    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    //}

    //else
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = MipLevels;
        SRVDesc.Texture2D.MostDetailedMip = 0;
    }

    g_GfxManager.GetGfxDevice().Dev()->CreateShaderResourceView(GetD3D12Resource(), &SRVDesc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}

void GfxTexture::CreateUAV(const InitParams& initParams)
{
    assert(initParams.m_Dimension == D3D12_RESOURCE_DIMENSION_BUFFER);
    assert(initParams.m_BufferParams.m_NumElements != 0);
    assert(initParams.m_BufferParams.m_StructureByteStride != 0);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = initParams.m_BufferParams.m_NumElements;
    uavDesc.Buffer.StructureByteStride = initParams.m_BufferParams.m_StructureByteStride;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE; // TOOD: D3D12_BUFFER_UAV_FLAG_RAW

    g_GfxManager.GetGfxDevice().Dev()->CreateUnorderedAccessView(GetD3D12Resource(), nullptr, &uavDesc, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}

void GfxTexture::InitializeForSwapChain(ComPtr<IDXGISwapChain4>& swapChain, DXGI_FORMAT format, uint32_t backBufferIdx)
{
    bbeProfileFunction();

    assert(!m_D3D12MABufferAllocation); // No D3D12MA allocation needed
    assert(swapChain);
    assert(format != DXGI_FORMAT_UNKNOWN);
    assert(backBufferIdx <= 3); // you dont need more than 3 back buffers...

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    m_InitParams.m_Format = format;

    DX12_CALL(swapChain->GetBuffer(backBufferIdx, IID_PPV_ARGS(&m_HazardTrackedResource)));

    m_GfxDescriptorHeap.Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1);

    m_HazardTrackedResource->SetName(StringUtils::Utf8ToWide(StringFormat("Back Buffer RTV %d", backBufferIdx)));

    gfxDevice.Dev()->CreateRenderTargetView(m_HazardTrackedResource, nullptr, m_GfxDescriptorHeap.Dev()->GetCPUDescriptorHandleForHeapStart());
}
