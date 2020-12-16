#pragma once

class GfxContext;

class GfxRendererBase
{
public:
    virtual void Initialize() {};
    virtual bool ShouldPopulateCommandList(GfxContext&) const { return true; }
    virtual void PopulateCommandList(GfxContext& context) {};
    const char* GetName() const { return m_Name; }

    static void RegisterRenderer(GfxRendererBase* renderer, std::string_view name);
    inline static std::vector<GfxRendererBase*> ms_AllRenderers;

private:
    const char* m_Name;
};

#define REGISTER_RENDERER(TYPE)                                         \
static TYPE gs_##TYPE;                                                  \
GfxRendererBase* g_##TYPE = &gs_##TYPE;                                 \
struct TYPE##_Registerer                                                \
{                                                                       \
    TYPE##_Registerer()                                                 \
    {                                                                   \
        GfxRendererBase::RegisterRenderer(g_##TYPE, bbeTOSTRING(TYPE)); \
    }                                                                   \
};                                                                      \
static TYPE##_Registerer gs_##TYPE##_Registerer;
