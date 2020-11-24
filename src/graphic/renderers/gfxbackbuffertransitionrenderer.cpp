
#include <graphic/pch.h>

class GfxBackBufferTransitionRenderer : public GfxRendererBase
{
    // Inherited via GfxRendererBase
    const char* GetName() const override { return "GfxBackBufferTransitionRenderer"; }
};

static GfxBackBufferTransitionRenderer gs_GfxBackBufferTransitionRenderer;
