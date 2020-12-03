#pragma once

class GfxContext;

class GfxRendererBase
{
public:
    virtual void Initialize() {};
    virtual void ShutDown() {};
    virtual bool ShouldPopulateCommandList(GfxContext&) const { return true; }
    virtual void PopulateCommandList(GfxContext& context) {};
    virtual const char* GetName() const = 0;

    static void RegisterRenderer(GfxRendererBase* renderer);
    inline static std::vector<GfxRendererBase*> ms_AllRenderers;
};

#define REGISTER_RENDERER(TYPE)                      \
static TYPE gs_##TYPE;                               \
GfxRendererBase* g_##TYPE = &gs_##TYPE;              \
struct TYPE##_Registerer                             \
{                                                    \
    TYPE##_Registerer()                              \
    {                                                \
        GfxRendererBase::RegisterRenderer(g_##TYPE); \
    }                                                \
};                                                   \
static TYPE##_Registerer gs_##TYPE##_Registerer;
