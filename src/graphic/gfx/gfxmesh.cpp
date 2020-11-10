#include <graphic/gfx/gfxmesh.h>

#include <graphic/gfx/gfxmanager.h>

void GfxMesh::Initialize(const GfxMesh::InitParams& initParams)
{
    bbeProfileFunction();

    assert(initParams.m_VertexFormat);
    m_VertexFormat = initParams.m_VertexFormat;

    m_VertexBuffer.Initialize(initParams.m_VBInitParams);
    m_IndexBuffer.Initialize(initParams.m_IBInitParams);
}

void GfxMesh::Release()
{
    m_VertexBuffer.Release();
    m_IndexBuffer.Release();
}
