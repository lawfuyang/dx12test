#pragma once

#include "graphic/renderpasses/gfxrenderpass.h"

class GfxTestRenderPass : public GfxRenderPass
{
public:
    GfxTestRenderPass();

    void Render(GfxContext&) override;

private:
};
