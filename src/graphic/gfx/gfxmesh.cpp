#include <graphic/gfx/gfxmesh.h>
#include <graphic/pch.h>

void GfxMesh::Initialize(const GfxMesh::InitParams& initParams)
{
    bbeProfileFunction();

    assert(initParams.m_VertexFormat);
    m_VertexFormat = initParams.m_VertexFormat;

    m_IndexBuffer.Initialize(initParams.m_IBInitParams);
}

void GfxMesh::Release()
{
    m_VertexBuffer.Release();
    m_IndexBuffer.Release();
}
