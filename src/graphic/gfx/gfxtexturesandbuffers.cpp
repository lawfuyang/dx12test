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

void GfxHeap::Release(D3D12MA::Allocation* allocation)
{
    bbeLogGfxAlloc("Destroying D3D12MA::Allocation '{}'", StringUtils::WideToUtf8(allocation->GetName()));

    UNTRACK_GFX_ALLOC_REF_COUNT(allocation);

    allocation->GetResource()->Release();
    allocation->Release();
}

D3D12MA::Allocation* GfxHeap::Create(D3D12_HEAP_TYPE heapType, CD3DX12_RESOURCE_DESC1&& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue, std::string_view debugName)
{
    bbeProfileFunction();

    D3D12MA::ALLOCATION_DESC bufferAllocDesc{ D3D12MA::ALLOCATION_FLAG_WITHIN_BUDGET, heapType };

    static ID3D12ProtectedResourceSession* ProtectedSession = nullptr; // TODO
    D3D12MA::Allocation* allocHandle = nullptr;
    D3D12Resource* newHeap = nullptr;
    DX12_CALL(g_GfxMemoryAllocator.Dev().CreateResource2(
        &bufferAllocDesc,
        &desc,
        initialState,
        clearValue,
        ProtectedSession,
        &allocHandle,
        IID_PPV_ARGS(&newHeap)));

    assert(allocHandle);
    assert(newHeap);

    const StaticWString<128> resourceNameW = StringUtils::Utf8ToWide(debugName.data());
    allocHandle->SetName(resourceNameW.c_str());
    allocHandle->GetResource()->SetName(resourceNameW.c_str());

    StaticString<128> heapName = debugName.data();
    heapName += " Heap";
    SetD3DDebugName(allocHandle->GetHeap(), heapName.c_str());
    SetD3DDebugName(newHeap, heapName.c_str());

    TRACK_GFX_ALLOC_REF_COUNT(allocHandle);
    bbeLogGfxAlloc("Created D3D12MA::Allocation '{}'", debugName.data());

    return allocHandle;
}

void GfxHeap::UploadInitData(GfxCommandList& cmdList, D3D12Resource* destResource, uint32_t uploadBufferSize, uint32_t rowPitch, uint32_t slicePitch, const void* initData, std::string_view debugName)
{
    // create upload heap to hold upload init data
    StaticString<256> nameBuffer = debugName.data();
    nameBuffer += " Upload Buffer";

    D3D12MA::Allocation* uploadHeapAlloc = GfxHeap::Create(D3D12_HEAP_TYPE_UPLOAD, CD3DX12_RESOURCE_DESC1::Buffer(uploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, debugName);

    // upload init data via CopyTextureRegion/CopyBufferRegion
    const UINT MaxSubresources = 1;
    const UINT64 IntermediateOffset = 0;
    const UINT FirstSubresource = 0;
    const UINT NumSubresources = 1;
    const D3D12_SUBRESOURCE_DATA data{ initData, rowPitch, slicePitch };
    const UINT64 r = UpdateSubresources<MaxSubresources>(cmdList.Dev(), destResource, uploadHeapAlloc->GetResource(), IntermediateOffset, FirstSubresource, NumSubresources, &data);
    assert(r);

    // we don't need this upload heap anymore... release it 1 frame later.
    g_GfxManager.AddGraphicCommand([uploadHeapAlloc]() { GfxHeap::Release(uploadHeapAlloc); });
}

void GfxHazardTrackedResource::Release()
{
    bbeProfileFunction();

    if (m_D3D12MABufferAllocation)
    {
        GfxHeap::Release(m_D3D12MABufferAllocation);
        m_D3D12MABufferAllocation = nullptr;
    }
}

void GfxVertexBuffer::Initialize(const InitParams& initParams)
{
    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName.c_str());
    Initialize(initContext, initParams);
}

void GfxVertexBuffer::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);
    assert(!m_D3D12MABufferAllocation);

    assert(initParams.m_NumVertices > 0);
    assert(initParams.m_VertexSize > 0);

    m_StrideInBytes = initParams.m_VertexSize;
    m_NumVertices = initParams.m_NumVertices;
    const uint32_t sizeInBytes = initParams.m_VertexSize * initParams.m_NumVertices;

    if (sizeInBytes > BBE_MB(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM))
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
    m_D3D12MABufferAllocation = GfxHeap::Create(initParams.m_HeapType, CD3DX12_RESOURCE_DESC1::Buffer(sizeInBytes), hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initialState, nullptr, initParams.m_ResourceName.c_str());
    assert(m_D3D12MABufferAllocation);
    m_D3D12Resource = m_D3D12MABufferAllocation->GetResource();

    if (hasInitData)
    {
        GfxHeap::UploadInitData(initContext.GetCommandList(), m_D3D12Resource, sizeInBytes, sizeInBytes, sizeInBytes, initParams.m_InitData, initParams.m_ResourceName.c_str());

        // after uploading init values, transition the vertex buffer data from copy destination state to vertex buffer state
        initContext.TransitionResource(*this, initialState, true);
    }

    g_GfxCommandListsManager.QueueCommandListToExecute(&initContext.GetCommandList());
}

void GfxVertexBuffer::Initialize(uint32_t numVertices, uint32_t vertexSize, const void* initData, std::string_view debugName)
{
    assert(numVertices > 0);
    assert(vertexSize > 0);

    bbeProfileFunction();

    const uint32_t sizeInBytes = numVertices * vertexSize;
    assert(sizeInBytes <= BBE_MB(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM));

    GfxCommandList& newCmdList = *g_GfxCommandListsManager.GetMainQueue().AllocateCommandList(debugName);


}

void GfxIndexBuffer::Initialize(const InitParams& initParams)
{
    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.m_ResourceName.c_str());
    Initialize(initContext, initParams);
}

void GfxIndexBuffer::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(initContext);

    assert(!m_D3D12MABufferAllocation);
    assert(initParams.m_NumIndices);

    m_NumIndices = initParams.m_NumIndices;
    const uint32_t sizeInBytes = initParams.m_NumIndices * 2;

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
    m_D3D12MABufferAllocation = GfxHeap::Create(initParams.m_HeapType, CD3DX12_RESOURCE_DESC1::Buffer(sizeInBytes), hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initialState, nullptr, initParams.m_ResourceName.c_str());
    assert(m_D3D12MABufferAllocation);
    m_D3D12Resource = m_D3D12MABufferAllocation->GetResource();

    if (hasInitData)
    {
        GfxHeap::UploadInitData(initContext.GetCommandList(), m_D3D12Resource, sizeInBytes, sizeInBytes, sizeInBytes, initParams.m_InitData, initParams.m_ResourceName.c_str());

        // after uploading init values, transition the index buffer data from copy destination state to index buffer state
        initContext.TransitionResource(*this, initialState, true);
    }

    g_GfxCommandListsManager.QueueCommandListToExecute(&initContext.GetCommandList());
}

ForwardDeclareSerializerFunctions(GfxTexture);

CD3DX12_RESOURCE_DESC1 GfxTexture::GetDescForGfxTexture(const GfxTexture::InitParams& i)
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
        return CD3DX12_RESOURCE_DESC1::Tex2D(i.m_Format, i.m_TexParams.m_Width, i.m_TexParams.m_Height, ArraySize, MipLevels, 1, 0, i.m_Flags);
    }
    case D3D12_RESOURCE_DIMENSION_BUFFER:
    {
        const uint32_t width = i.m_BufferParams.m_NumElements * i.m_BufferParams.m_StructureByteStride;
        static const float s_SizeLimitMb = [] 
        {
            D3D12MA::Budget gpuBudget{};
            g_GfxMemoryAllocator.Dev().GetBudget(&gpuBudget, nullptr);

            static constexpr float A_TERM = 128.0f;
            static constexpr float B_TERM = 0.25f;
            static constexpr float C_TERM = 2048.0f;
            static_assert(A_TERM == D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM);
            static_assert(B_TERM == D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_B_TERM);
            static_assert(C_TERM == D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM);

            return std::min(std::max(A_TERM, B_TERM * BBE_MB(gpuBudget.BudgetBytes)), C_TERM);
        }();

        assert(i.m_Format == DXGI_FORMAT_UNKNOWN);
        assert(BBE_TO_MB(width) < s_SizeLimitMb);

        return CD3DX12_RESOURCE_DESC1::Buffer(width, i.m_Flags);
    }
    }

    assert(false);
    return *(CD3DX12_RESOURCE_DESC1*)0;
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

    m_Format = initParams.m_Format;

    CD3DX12_RESOURCE_DESC1 desc = GetDescForGfxTexture(initParams);

    UINT64 rowSizeInBytes = 0;
    uint32_t sizeInBytes = 0;
    {
        UINT FirstSubresource = 0;
        UINT NumSubresources = 1;
        const UINT64 BaseOffset = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts{};
        UINT numRows = 0;
        UINT64 totalBytes = 0;
        g_GfxManager.GetGfxDevice().Dev()->GetCopyableFootprints1(&desc, FirstSubresource, NumSubresources, BaseOffset, &layouts, &numRows, &rowSizeInBytes, &totalBytes);

        sizeInBytes = (uint32_t)totalBytes;
    }

    const bool hasInitData = initParams.m_InitData != nullptr;

    // if we have init data, we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
    m_CurrentResourceState = hasInitData ? D3D12_RESOURCE_STATE_COPY_DEST : initParams.m_InitialState;

    const bool hasClearValue = (initParams.m_ClearValue.Format != DXGI_FORMAT_UNKNOWN) &&
                               (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) && 
                               ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == 0);

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = GfxHeap::Create(D3D12_HEAP_TYPE_DEFAULT, std::move(desc), m_CurrentResourceState, hasClearValue ? &initParams.m_ClearValue : nullptr, initParams.m_ResourceName.c_str());
    assert(m_D3D12MABufferAllocation);
    m_D3D12Resource = m_D3D12MABufferAllocation->GetResource();

    // if init data is specified, upload it
    if (hasInitData)
    {
        GfxHeap::UploadInitData(initContext.GetCommandList(), m_D3D12Resource, sizeInBytes, (uint32_t)rowSizeInBytes, (uint32_t)(rowSizeInBytes * desc.Height), initParams.m_InitData, initParams.m_ResourceName.c_str());

        // after uploading init values, transition the texture from copy destination state to common state
        initContext.TransitionResource(*this, initParams.m_InitialState, true);
    }

    SetD3DDebugName(GetD3D12Resource(), initParams.m_ResourceName.c_str());

    g_GfxCommandListsManager.QueueCommandListToExecute(&initContext.GetCommandList());
}

void GfxTexture::UpdateIMGUI()
{
    ScopedIMGUIID scopedID{ this };

    D3D12Resource* gfxResource = m_D3D12MABufferAllocation->GetResource();
    D3D12_RESOURCE_DESC desc = gfxResource->GetDesc();

    StaticString<256> nameBuffer = GetD3DDebugName(gfxResource);
    ImGui::InputText("Name", nameBuffer.data(), nameBuffer.capacity(), ImGuiInputTextFlags_ReadOnly);

    const bbeVector2U dims{ (uint32_t)desc.Width, desc.Height };
    ImGui::InputInt2("Dimensions", (int*)&dims, ImGuiInputTextFlags_ReadOnly);
}
