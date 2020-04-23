#pragma once

#include <graphic/gfxtexturesandbuffers.h>

class GfxMesh
{
public:
    struct InitParams
    {
        std::string& MeshName;
        GfxVertexBuffer::InitParams m_VBInitParams;
        GfxIndexBuffer::InitParams m_IBInitParams;
    };

    void Initialize(const InitParams&);
    void Initialize(GfxContext& initContext, const InitParams&);

    GfxVertexBuffer& GetVertexBuffer() { return m_VertexBuffer; }
    GfxIndexBuffer& GetIndexBuffer() { return m_IndexBuffer; }

private:
    GfxVertexBuffer m_VertexBuffer;
    GfxIndexBuffer m_IndexBuffer;
};
