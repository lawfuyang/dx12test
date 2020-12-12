#include <graphic/gfx/gfxmesh.h>
#include <graphic/pch.h>

void GfxMesh::Release()
{
    m_VertexBuffer.Release();
    m_IndexBuffer.Release();
}
