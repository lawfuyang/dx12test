#pragma once

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxVertexFormat;

class GfxMesh
{
public:
    struct InitParams
    {
        std::string MeshName;
        GfxVertexFormat* m_VertexFormat = nullptr;
        GfxVertexBuffer::InitParams m_VBInitParams;
        GfxIndexBuffer::InitParams m_IBInitParams;
    };

    void Initialize(const InitParams&);

    void Release();

    GfxVertexBuffer& GetVertexBuffer() { return m_VertexBuffer; }
    GfxIndexBuffer& GetIndexBuffer() { return m_IndexBuffer; }
    GfxVertexFormat& GetVertexFormat() { return *m_VertexFormat; }

private:
    GfxVertexBuffer m_VertexBuffer;
    GfxIndexBuffer m_IndexBuffer;
    GfxVertexFormat* m_VertexFormat = nullptr;
};
