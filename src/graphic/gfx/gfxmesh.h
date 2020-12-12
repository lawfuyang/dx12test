#pragma once

#include <graphic/gfx/gfxtexturesandbuffers.h>

class GfxVertexFormat;

struct GfxMesh
{
    void Release();

    GfxVertexBuffer m_VertexBuffer;
    GfxIndexBuffer m_IndexBuffer;
    GfxVertexFormat* m_VertexFormat = nullptr;
};
