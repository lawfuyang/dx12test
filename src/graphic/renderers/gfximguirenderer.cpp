#include <graphic/renderers/GfxIMGUIRenderer.h>

#include <system/imguimanager.h>

#include <graphic/gfx/gfxmanager.h>
#include <graphic/gfx/gfxcontext.h>

#include <tmp/shaders/autogen/cpp/IMGUIConsts.h>
#include <tmp/shaders/autogen/cpp/VS_IMGUI.h>
#include <tmp/shaders/autogen/cpp/PS_IMGUI.h>

static const uint32_t gs_VBufferGrowSize = 5000;
static const uint32_t gs_IBufferGrowSize = 10000;

static_assert(IsAligned(gs_IBufferGrowSize, 4));

void GfxIMGUIRenderer::Initialize()
{
    bbeProfileFunction();

    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxIMGUIRenderer::Initialize");

    // Build texture atlas
    InitFontsTexture(initContext);

    // Create buffers with a default size (they will later be grown as needed)
    void* dummyDrawData = nullptr;
    GrowBuffers(initContext, *(const IMGUIDrawData*)dummyDrawData);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());

    // Keep ranges static so GfxContext can parse through them
    static CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParams[2];
    rootParams[0].InitAsDescriptorTable(1, &ranges[0]);
    rootParams[1].InitAsDescriptorTable(1, &ranges[1]);

    m_RootSignature.Compile(rootParams, _countof(rootParams), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "GfxIMGUIRenderer_RootSignature");
}

void GfxIMGUIRenderer::InitFontsTexture(GfxContext& context)
{
    bbeProfileFunction();

    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    GfxTexture::InitParams initParams;
    initParams.m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    initParams.m_Width = width;
    initParams.m_Height = height;
    initParams.m_InitData = pixels;
    initParams.m_ResourceName = "IMGUI Fonts Texture";

    m_FontsTexture.Initialize(context, initParams);
}

void GfxIMGUIRenderer::GrowBuffers(GfxContext& context, const IMGUIDrawData& imguiDrawData)
{
    bbeProfileFunction();
    
    // empty buffers == init phase
    const bool isInitPhase = (m_VertexBuffer.GetNumVertices() == 0) && (m_IndexBuffer.GetNumIndices() == 0);

    StaticString<128> bufferNames;

    // Create and grow vertex/index buffers if needed
    // since we will be continuously streaming vertex/index via "Map", the heap type must be "D3D12_HEAP_TYPE_UPLOAD"
    if (isInitPhase || (m_VertexBuffer.GetNumVertices() < imguiDrawData.m_VtxCount))
    {
        m_VertexBuffer.Release();

        GfxVertexBuffer::InitParams initParams;
        initParams.m_InitData = nullptr;
        initParams.m_NumVertices = isInitPhase ? gs_VBufferGrowSize : imguiDrawData.m_VtxCount + gs_VBufferGrowSize;
        initParams.m_VertexSize = sizeof(ImDrawVert);
        initParams.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;

        bufferNames = "IMGUI GfxVertexBuffer" + StringFormat("_%u", initParams.m_NumVertices);
        initParams.m_ResourceName = bufferNames.c_str();

        m_VertexBuffer.Initialize(context, initParams);
    }
    if (isInitPhase || (m_IndexBuffer.GetNumIndices() < imguiDrawData.m_IdxCount))
    {
        m_IndexBuffer.Release();

        GfxIndexBuffer::InitParams initParams;
        initParams.m_InitData = nullptr;
        initParams.m_NumIndices = isInitPhase ? gs_IBufferGrowSize : AlignUp(imguiDrawData.m_IdxCount + gs_IBufferGrowSize, 4); // the indexbuffer copy requires alignment
        initParams.m_IndexSize = sizeof(uint16_t);
        initParams.m_HeapType = D3D12_HEAP_TYPE_UPLOAD;

        bufferNames = "IMGUI GfxIndexBuffer" + StringFormat("_%u", initParams.m_NumIndices);
        initParams.m_ResourceName = bufferNames.c_str();

        m_IndexBuffer.Initialize(context, initParams);
    }
}

void GfxIMGUIRenderer::UploadBufferData(const IMGUIDrawData& imguiDrawData)
{
    ImDrawVert* vtx_dst;
    ImDrawIdx* idx_dst;
    D3D12_RANGE range = {};
    DX12_CALL(m_VertexBuffer.GetD3D12Resource()->Map(0, &range, &(void*)vtx_dst));
    DX12_CALL(m_IndexBuffer.GetD3D12Resource()->Map(0, &range, &(void*)idx_dst));

    for (uint32_t n = 0; n < imguiDrawData.m_DrawList.size(); n++)
    {
        const IMGUICmdList& cmd_list = imguiDrawData.m_DrawList[n];
        memcpy(vtx_dst, cmd_list.m_VB.data(), cmd_list.m_VB.size() * sizeof(ImDrawVert));
        memcpy(idx_dst, cmd_list.m_IB.data(), cmd_list.m_IB.size() * sizeof(ImDrawIdx));
        vtx_dst += cmd_list.m_VB.size();
        idx_dst += cmd_list.m_IB.size();
    }
    m_VertexBuffer.GetD3D12Resource()->Unmap(0, &range);
    m_IndexBuffer.GetD3D12Resource()->Unmap(0, &range);
}

void GfxIMGUIRenderer::SetupRenderStates(GfxContext& context, const IMGUIDrawData& imguiDrawData)
{
    GfxPipelineStateObject& pso = context.GetPSO();

    {
        CD3DX12_BLEND_DESC& desc = pso.GetBlendStates();
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    {
        CD3DX12_RASTERIZER_DESC& desc = pso.GetRasterizerStates();
        desc.FillMode = D3D12_FILL_MODE_SOLID;
        desc.CullMode = D3D12_CULL_MODE_NONE;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        desc.DepthClipEnable = true;
        desc.MultisampleEnable = FALSE;
        desc.AntialiasedLineEnable = FALSE;
        desc.ForcedSampleCount = 0;
        desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }

    {
        CD3DX12_DEPTH_STENCIL_DESC1& desc = pso.GetDepthStencilStates();
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.StencilEnable = false;
        desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.BackFace = desc.FrontFace;
    }

    // Setup viewport
    {
        D3D12_VIEWPORT vp;
        vp.Width = imguiDrawData.m_Size.x;
        vp.Height = imguiDrawData.m_Size.y;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = vp.TopLeftY = 0.0f;
        context.GetCommandList().Dev()->RSSetViewports(1, &vp);
    }
}

void GfxIMGUIRenderer::ShutDown()
{
    m_VertexBuffer.Release();
    m_IndexBuffer.Release();
    m_FontsTexture.Release();
}

void GfxIMGUIRenderer::PopulateCommandList()
{
    bbeProfileFunction();

    GfxContext& context = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, "GfxIMGUIRenderer");
    m_Context = &context;
    context.SetRootSignature(m_RootSignature);

    bbeProfileGPUFunction(context);

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
    GrowBuffers(context, imguiDrawData);

    // Upload vertex/index data into a single contiguous GPU buffer
    UploadBufferData(imguiDrawData);

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
        //m_ConstantBuffer.Update(&consts);

        GfxConstantBuffer constsCB{ 0 };
        constsCB.UploadToGPU(context, consts);
    }

    GfxPipelineStateObject& pso = context.GetPSO();
    pso.SetVertexShader(g_GfxShaderManager.GetShader(Shaders::VS_IMGUIPermutations{}));
    pso.SetPixelShader(g_GfxShaderManager.GetShader(Shaders::PS_IMGUIPermutations{}));
    pso.SetVertexFormat(GfxDefaultVertexFormats::Position2f_TexCoord2f_Color4ub);

    context.StageSRV(m_FontsTexture, 1, 0);

    context.SetVertexBuffer(m_VertexBuffer);
    context.SetIndexBuffer(m_IndexBuffer);
    context.SetRenderTarget(0, g_GfxManager.GetSwapChain().GetCurrentBackBuffer());

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
            D3D12_RECT r;
            r.left = (LONG)(cmd.ClipRect.x - clip_off.x);
            r.top = (LONG)(cmd.ClipRect.y - clip_off.y);
            r.right = (LONG)(cmd.ClipRect.z - clip_off.x);
            r.bottom = (LONG)(cmd.ClipRect.w - clip_off.y);
            context.GetCommandList().Dev()->RSSetScissorRects(1, &r);

            context.DrawIndexedInstanced(cmd.ElemCount, 1, cmd.IdxOffset + global_idx_offset, cmd.VtxOffset + global_vtx_offset, 0);

        }
        global_idx_offset += cmd_list.m_IB.size();
        global_vtx_offset += cmd_list.m_VB.size();
    }
}
