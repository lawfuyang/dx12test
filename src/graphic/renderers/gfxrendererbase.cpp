#include <graphic/renderers/gfxrendererbase.h>
#include <graphic/pch.h>

void GfxRendererBase::RegisterRenderer(GfxRendererBase* renderer, std::string_view name)
{
    bbeMultiThreadDetector();

    renderer->m_Name = name.data();
    ms_AllRenderers.push_back(renderer);
}
