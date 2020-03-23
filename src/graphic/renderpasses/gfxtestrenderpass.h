#pragma once

#include "graphic/renderpasses/gfxrenderpass.h"

#include "graphic/gfxtexturesandbuffers.h"

class GfxContext;

class GfxTestRenderPass : public GfxRenderPass
{
public:
    GfxTestRenderPass();

    void Initialize();
    void ShutDown() override;
    void Render(GfxContext&) override;

private:
    GfxVertexBuffer   m_QuadVBuffer;
    GfxIndexBuffer    m_QuadIBuffer;
    GfxConstantBuffer m_RenderPassCB;
};
