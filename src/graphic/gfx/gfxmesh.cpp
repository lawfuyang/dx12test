#include <graphic/gfx/gfxmesh.h>

#include <graphic/gfx/gfxmanager.h>

void GfxMesh::Initialize(const GfxMesh::InitParams& initParams)
{
    GfxContext& initContext = g_GfxManager.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.MeshName);

    Initialize(initContext, initParams);

    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    gfxDevice.GetCommandListsManager().QueueCommandListToExecute(initContext.GetCommandList(), initContext.GetCommandList().GetType());
}

void GfxMesh::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();

    assert(initParams.m_VertexFormat);
    m_VertexFormat = initParams.m_VertexFormat;

    m_VertexBuffer.Initialize(initContext, initParams.m_VBInitParams);
    m_IndexBuffer.Initialize(initContext, initParams.m_IBInitParams);
}

void GfxMesh::Release()
{
    m_VertexBuffer.Release();
    m_IndexBuffer.Release();
}
