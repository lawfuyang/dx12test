#include <graphic/renderers/gfxrendererbase.h>
#include <graphic/pch.h>

void GfxRendererBase::RegisterRenderer(GfxRendererBase* renderer)
{
    bbeMultiThreadDetector();

    ms_AllRenderers.push_back(renderer);
}
