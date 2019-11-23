#pragma once

#include "graphic/renderpasses/gfxrenderpass.h"

class GfxTestRenderPass : public GfxRenderPass
{
public:
    GfxTestRenderPass();

    void Render(const GfxContext&) override;

private:
};
