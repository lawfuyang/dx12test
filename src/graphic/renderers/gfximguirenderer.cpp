#include <graphic/pch.h>

#include <system/imguimanager.h>

#include <tmp/shaders/autogen/cpp/ShaderInputs/IMGUIConsts.h>
#include <tmp/shaders/autogen/cpp/ShaderPermutations/IMGUI.h>

static const uint32_t gs_VBufferGrowSize = 5000;
static const uint32_t gs_IBufferGrowSize = 10000;

static_assert(IsAligned(gs_IBufferGrowSize, 4));

class GfxIMGUIRenderer : public GfxRendererBase
{
    void Initialize() override;
    void PopulateCommandList(GfxContext& context) override;

    void InitFontsTexture();
    void GrowBuffers(const IMGUIDrawData& imguiDrawData);
    void UploadBufferData(GfxContext& context, const IMGUIDrawData& imguiDrawData);
    void SetupRenderStates(GfxContext&, const IMGUIDrawData& imguiDrawData);

    GfxRootSignature* m_RootSignature = nullptr;
    GfxVertexBuffer   m_VertexBuffer;
    GfxIndexBuffer    m_IndexBuffer;
    GfxTexture        m_FontsTexture;
};

void GfxIMGUIRenderer::Initialize()
{
    bbeProfileFunction();

    // Build texture atlas
    InitFontsTexture();

    // Create buffers with a default size (they will later be grown as needed)
    GrowBuffers(*(const IMGUIDrawData*)0);

    // Perfomance TIP: Order from most frequent to least frequent.
    CD3DX12_DESCRIPTOR_RANGE1 ranges[2]{};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, AutoGenerated::IMGUIConsts::NbSRVs, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

    m_RootSignature = g_GfxRootSignatureManager.GetOrCreateRootSig(ranges, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "GfxIMGUIRenderer_RootSignature");
}

void GfxIMGUIRenderer::InitFontsTexture()
{
    bbeProfileFunction();

    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    m_FontsTexture.InitializeTexture(CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height), pixels);
    m_FontsTexture.SetDebugName("IMGUI Fonts Texture");
}

void GfxIMGUIRenderer::GrowBuffers(const IMGUIDrawData& imguiDrawData)
{
    bbeProfileFunction();
    
    // empty buffers == init phase
    const bool isInitPhase = (m_VertexBuffer.GetNumVertices() == 0) && (m_IndexBuffer.GetNumIndices() == 0);

    // Create and grow vertex/index buffers if needed
    // since we will be continuously streaming vertex/index via "Map", the heap type must be "D3D12_HEAP_TYPE_UPLOAD"
    if (isInitPhase || (m_VertexBuffer.GetNumVertices() < imguiDrawData.m_VtxCount))
    {
        m_VertexBuffer.Release();

        const uint32_t numVertices = isInitPhase ? gs_VBufferGrowSize : imguiDrawData.m_VtxCount + gs_VBufferGrowSize;
        m_VertexBuffer.Initialize(numVertices, sizeof(ImDrawVert), nullptr);
        m_VertexBuffer.SetDebugName("IMGUI GfxVertexBuffer");
    }
    if (isInitPhase || (m_IndexBuffer.GetNumIndices() < imguiDrawData.m_IdxCount))
    {
        m_IndexBuffer.Release();

        const uint32_t numIndices = isInitPhase ? gs_IBufferGrowSize : AlignUp(imguiDrawData.m_IdxCount + gs_IBufferGrowSize, 4); // the indexbuffer copy requires alignment
        m_IndexBuffer.Initialize(numIndices, nullptr);
        m_VertexBuffer.SetDebugName("IMGUI GfxIndexBuffer");
    }
}

void GfxIMGUIRenderer::UploadBufferData(GfxContext& context, const IMGUIDrawData& imguiDrawData)
{
    std::vector<ImDrawVert> vertices;
    std::vector<ImDrawIdx> indices;
    vertices.resize(m_VertexBuffer.GetNumVertices());
    indices.resize(m_IndexBuffer.GetNumIndices());

    ImDrawVert* vtx_dst = vertices.data();
    ImDrawIdx* idx_dst = indices.data();

    for (uint32_t n = 0; n < imguiDrawData.m_DrawList.size(); n++)
    {
        const IMGUICmdList& cmd_list = imguiDrawData.m_DrawList[n];
        memcpy(vtx_dst, cmd_list.m_VB.data(), cmd_list.m_VB.size() * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list.m_IB.data(), cmd_list.m_IB.size() * sizeof(ImDrawIdx));
        vtx_dst += cmd_list.m_VB.size();
        idx_dst += cmd_list.m_IB.size();
    }

    uint32_t uploadSize = vertices.size() * sizeof(ImDrawVert);
    UploadToGfxResource(context.GetCommandList().Dev(), m_VertexBuffer, uploadSize, uploadSize, uploadSize, vertices.data());

    uploadSize = indices.size() * sizeof(ImDrawIdx);
    UploadToGfxResource(context.GetCommandList().Dev(), m_IndexBuffer, uploadSize, uploadSize, uploadSize, indices.data());
}

void GfxIMGUIRenderer::SetupRenderStates(GfxContext& context, const IMGUIDrawData& imguiDrawData)
{
    D3D12_RENDER_TARGET_BLEND_DESC desc{};
    desc.BlendEnable = true;
    desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    desc.BlendOp = D3D12_BLEND_OP_ADD;
    desc.SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    desc.DestBlendAlpha = D3D12_BLEND_ZERO;
    desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    context.SetBlendStates(0, desc);

    context.SetRasterizerStates(GfxCommonStates::CullNone);
    context.SetDepthStencilStates(GfxCommonStates::DepthNone);

    // Setup viewport
    {
        D3D12_VIEWPORT vp{};
        vp.Width = imguiDrawData.m_Size.x;
        vp.Height = imguiDrawData.m_Size.y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0.0f;
        context.SetViewport(vp);
    }
}

void GfxIMGUIRenderer::PopulateCommandList(GfxContext& context)
{
    bbeProfileFunction();
    bbeProfileGPUFunction(context);

    assert(m_RootSignature);
    context.SetRootSignature(*m_RootSignature);

    static_assert(sizeof(ImDrawVert) == sizeof(float) * 2 + sizeof(float) * 2 + sizeof(uint32_t)); // Position2f_TexCoord2f_Color4ub
    static_assert(sizeof(ImDrawIdx) == sizeof(uint16_t)); // 2 byte index size

    const IMGUIDrawData imguiDrawData = g_IMGUIManager.GetDrawData();

    // Avoid rendering when minimized
    if (imguiDrawData.m_Size.x <= 0.0f || imguiDrawData.m_Size.y <= 0.0f)
        return;

    // nothing to draw. return.
    if (imguiDrawData.m_DrawList.size() == 0)
        return;

    // Create and grow vertex/index buffers if needed
    GrowBuffers(imguiDrawData);

    // Upload vertex/index data into a single contiguous GPU buffer
    UploadBufferData(context, imguiDrawData);

    SetupRenderStates(context, imguiDrawData);

    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right).
    {
        const float L = imguiDrawData.m_Pos.x;
        const float R = imguiDrawData.m_Pos.x + imguiDrawData.m_Size.x;
        const float T = imguiDrawData.m_Pos.y;
        const float B = imguiDrawData.m_Pos.y + imguiDrawData.m_Size.y;

        bbeMatrix mvp
        {
            2.0f / (R - L)   , 0.0f,              0.0f, 0.0f,
            0.0f             , 2.0f / (T - B),    0.0f, 0.0f,
            0.0f             , 0.0f,              0.5f, 0.0f,
            (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f
        };
        mvp = mvp.Transpose();

        AutoGenerated::IMGUIConsts consts;
        consts.m_ProjMatrix = mvp;
        context.StageCBV(consts);
    }

    context.SetShader(g_GfxShaderManager.GetShader(Shaders::VS_IMGUI{}));
    context.SetShader(g_GfxShaderManager.GetShader(Shaders::PS_IMGUI{}));

    context.SetVertexFormat(GfxDefaultVertexFormats::Position2f_TexCoord2f_Color4ub);

    context.StageSRV(m_FontsTexture, 1, 0);

    context.SetVertexBuffer(m_VertexBuffer);
    context.SetIndexBuffer(m_IndexBuffer);
    context.SetRenderTarget(g_GfxManager.GetSwapChain().GetCurrentBackBuffer());

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    uint32_t global_vtx_offset = 0;
    uint32_t global_idx_offset = 0;
    const bbeVector2 clip_off = imguiDrawData.m_Pos;
    for (uint32_t n = 0; n < imguiDrawData.m_DrawList.size(); n++)
    {
        const IMGUICmdList& cmd_list = imguiDrawData.m_DrawList[n];
        for (uint32_t cmd_i = 0; cmd_i < cmd_list.m_DrawCmd.size(); cmd_i++)
        {
            const ImDrawCmd& cmd = cmd_list.m_DrawCmd[cmd_i];

            // Apply Scissor, Bind texture, Draw
            D3D12_RECT r{};
            r.left = (LONG)(cmd.ClipRect.x - clip_off.x);
            r.top = (LONG)(cmd.ClipRect.y - clip_off.y);
            r.right = (LONG)(cmd.ClipRect.z - clip_off.x);
            r.bottom = (LONG)(cmd.ClipRect.w - clip_off.y);
            context.SetRect(r);

            context.DrawIndexedInstanced(cmd.ElemCount, 1, cmd.IdxOffset + global_idx_offset, cmd.VtxOffset + global_vtx_offset);

        }
        global_idx_offset += cmd_list.m_IB.size();
        global_vtx_offset += cmd_list.m_VB.size();
    }
}

REGISTER_RENDERER(GfxIMGUIRenderer);
