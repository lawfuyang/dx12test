#pragma once

#include "graphic/renderpasses/gfxrenderpass.h"

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;

class GfxTestRenderPass : public GfxRenderPass
{
public:
    GfxTestRenderPass();

    void Initialize(GfxContext&);
    void ShutDown() override;
    void Render(GfxContext&) override;

private:
    GfxVertexBuffer m_QuadVBuffer;
    GfxIndexBuffer  m_QuadIBuffer;
};
