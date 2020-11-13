#include <graphic/renderers/gfxbodygravityparticles.h>

void GfxBodyGravityParticlesUpdate::Initialize()
{
}

void GfxBodyGravityParticlesUpdate::PopulateCommandList(GfxContext& context)
{
}

static GfxBodyGravityParticlesUpdate gs_GfxBodyGravityParticlesUpdate;
GfxRendererBase* g_GfxBodyGravityParticlesUpdate = &gs_GfxBodyGravityParticlesUpdate;


void GfxBodyGravityParticlesRender::Initialize()
{
}

void GfxBodyGravityParticlesRender::PopulateCommandList(GfxContext& context)
{
}

static GfxBodyGravityParticlesRender gs_GfxBodyGravityParticlesRender;
GfxRendererBase* g_GfxBodyGravityParticlesRender = &gs_GfxBodyGravityParticlesRender;
