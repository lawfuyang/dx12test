#include <graphic/gfxmesh.h>

#include <graphic/gfxmanager.h>

void GfxMesh::Initialize(const GfxMesh::InitParams& initParams)
{
    GfxDevice& gfxDevice = g_GfxManager.GetGfxDevice();
    GfxContext& initContext = gfxDevice.GenerateNewContext(D3D12_COMMAND_LIST_TYPE_DIRECT, initParams.MeshName);

    Initialize(initContext, initParams);
}

void GfxMesh::Initialize(GfxContext& initContext, const InitParams& initParams)
{
    bbeProfileFunction();

    m_VertexBuffer.Initialize(initContext, initParams.m_VBInitParams);
    m_IndexBuffer.Initialize(initContext, initParams.m_IBInitParams);
}
