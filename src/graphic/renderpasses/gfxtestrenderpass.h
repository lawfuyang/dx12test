#pragma once

#include "graphic/renderpasses/gfxrenderpass.h"

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;

class GfxTestRenderPass : public GfxRenderPass
{
public:
    GfxTestRenderPass(GfxContext& initContext);

    void Render(GfxContext&) override;

private:
    GfxVertexBuffer m_TriangleVBuffer;
};
