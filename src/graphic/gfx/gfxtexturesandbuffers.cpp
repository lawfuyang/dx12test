#include <graphic/gfx/gfxtexturesandbuffers.h>
#include <graphic/pch.h>

#include <system/imguimanager.h>

void GfxHazardTrackedResource::SetDebugName(std::string_view debugName) const
{
    if (m_D3D12Resource)
        SetD3DDebugName(m_D3D12Resource, debugName.data());
}

void GfxHazardTrackedResource::Release()
{
    bbeProfileFunction();

    if (m_D3D12MABufferAllocation)
    {
        g_GfxMemoryAllocator.ReleaseStatic(m_D3D12MABufferAllocation);
        m_D3D12MABufferAllocation = nullptr;
    }
}

void GfxVertexBuffer::Initialize(uint32_t numVertices, uint32_t vertexSize, const void* initData)
{
    assert(!m_D3D12MABufferAllocation);
    assert(numVertices > 0);
    assert(vertexSize > 0);

    bbeProfileFunction();

    const uint32_t sizeInBytes = numVertices * vertexSize;
    assert(sizeInBytes <= BBE_MB(D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_A_TERM));

    m_NumVertices = numVertices;
    m_StrideInBytes = vertexSize;

    const bool hasInitData = initData != nullptr;
    m_CurrentResourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = g_GfxMemoryAllocator.AllocateStatic(CD3DX12_RESOURCE_DESC1::Buffer(sizeInBytes), m_CurrentResourceState, nullptr);
    assert(m_D3D12MABufferAllocation);
    m_D3D12Resource = m_D3D12MABufferAllocation->GetResource();

    SetDebugName("Un-named GfxVertexBuffer");
    
    if (hasInitData)
    {
        GfxCommandList& newCmdList = *g_GfxCommandListsManager.GetMainQueue().AllocateCommandList("GfxVertexBuffer Upload Init Data");

        UploadToGfxResource(newCmdList.Dev(), *this, sizeInBytes, sizeInBytes, sizeInBytes, initData);

        g_GfxCommandListsManager.QueueCommandListToExecute(&newCmdList);
    }
}

void GfxIndexBuffer::Initialize(uint32_t numIndices, const void* initData)
{
    bbeProfileFunction();

    assert(!m_D3D12MABufferAllocation);
    assert(numIndices > 0);

    m_NumIndices = numIndices;
    const uint32_t sizeInBytes = numIndices * 2;

    m_CurrentResourceState = D3D12_RESOURCE_STATE_INDEX_BUFFER;

    const bool hasInitData = initData != nullptr;

    // Create heap to hold final buffer data
    m_D3D12MABufferAllocation = g_GfxMemoryAllocator.AllocateStatic(CD3DX12_RESOURCE_DESC1::Buffer(sizeInBytes), m_CurrentResourceState, nullptr);
    assert(m_D3D12MABufferAllocation);
    m_D3D12Resource = m_D3D12MABufferAllocation->GetResource();

    SetDebugName("Un-named GfxIndexBuffer");

    if (hasInitData)
    {
        GfxCommandList& newCmdList = *g_GfxCommandListsManager.GetMainQueue().AllocateCommandList("GfxIndexBuffer Upload Init Data");

        UploadToGfxResource(newCmdList.Dev(), *this, sizeInBytes, sizeInBytes, sizeInBytes, initData);

        g_GfxCommandListsManager.QueueCommandListToExecute(&newCmdList);
    }
}

ForwardDeclareSerializerFunctions(GfxTexture);

static void ResourceDescSanityCheck(const CD3DX12_RESOURCE_DESC1& desc)
{
    static const float BufferSizeLimitMb = []
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

    // TODO: D3D12_RESOURCE_DIMENSION_TEXTURE1D, D3D12_RESOURCE_DIMENSION_TEXTURE3D
    switch (desc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        assert(desc.Format != DXGI_FORMAT_UNKNOWN);
        assert(desc.Width > 0 && desc.Width <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
        assert(desc.Height > 0 && desc.Height <= D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION);
        // TODO: Texture2DArray
        // TODO: Mips
        break;

    case D3D12_RESOURCE_DIMENSION_BUFFER:
        assert(desc.Format == DXGI_FORMAT_UNKNOWN);
        assert(BBE_TO_MB(desc.Width) < BufferSizeLimitMb);
        break;
    }
}

void GfxTexture::Initialize(const CD3DX12_RESOURCE_DESC1& desc, const void* initData, D3D12_CLEAR_VALUE clearValue, D3D12_RESOURCE_STATES initialState)
{
    bbeProfileFunction();
    assert(!m_D3D12MABufferAllocation);

    m_Format = desc.Format;
    m_CurrentResourceState = initialState;

    // sanity check for resource desc
    ResourceDescSanityCheck(desc);

    const bool hasClearValue = (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) &&
                               ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) == 0) &&
                               (clearValue.Format != DXGI_FORMAT_UNKNOWN);

    // Create heap
    m_D3D12MABufferAllocation = g_GfxMemoryAllocator.AllocateStatic(desc, m_CurrentResourceState, hasClearValue ? &clearValue : nullptr);
    assert(m_D3D12MABufferAllocation);
    m_D3D12Resource = m_D3D12MABufferAllocation->GetResource();

    SetDebugName("Un-named GfxTexture");

    if (initData)
    {
        const UINT FirstSubresource = 0;
        const UINT NumSubresources = 1;
        const UINT64 BaseOffset = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts{};
        UINT numRows = 0;
        UINT64 rowSizeInBytes = 0;
        UINT64 totalBytes = 0;
        g_GfxManager.GetGfxDevice().Dev()->GetCopyableFootprints1(&desc, FirstSubresource, NumSubresources, BaseOffset, &layouts, &numRows, &rowSizeInBytes, &totalBytes);

        GfxCommandList& newCmdList = *g_GfxCommandListsManager.GetMainQueue().AllocateCommandList("GfxTexture Upload Init Data");
        UploadToGfxResource(newCmdList.Dev(), *this, (uint32_t)totalBytes, (uint32_t)rowSizeInBytes, (uint32_t)(rowSizeInBytes * desc.Height), initData);
        g_GfxCommandListsManager.QueueCommandListToExecute(&newCmdList);
    }
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
