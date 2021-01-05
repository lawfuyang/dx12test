#pragma once

class GfxContext;

class GfxRendererBase
{
public:
    virtual void Initialize() {};
    virtual D3D12_COMMAND_LIST_TYPE GetCommandListType(GfxContext&) const { return D3D12_COMMAND_LIST_TYPE_DIRECT; }
    virtual bool ShouldPopulateCommandList(GfxContext&) const { return true; }
    virtual void AddDependencies() {};
    virtual void PopulateCommandList(GfxContext& context) {};

    const char* GetName() const { return m_Name; }

    static void RegisterRenderer(GfxRendererBase* renderer, std::string_view name);
    inline static std::vector<GfxRendererBase*> ms_AllRenderers;

private:
    const char* m_Name;
    D3D12_COMMAND_LIST_TYPE m_Type;

    EventLockable m_Event;
    InplaceArray<GfxRendererBase*, 4> m_Dependencies;

    friend class GfxManager;
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
