#pragma once

class GfxContext;

class GfxRendererBase
{
public:
    virtual void Initialize() {};
    virtual void ShutDown() {};
    virtual bool ShouldRender(GfxContext&) const { return true; }
    virtual void PopulateCommandList(GfxContext& context) {};
    virtual const char* GetName() const = 0;
};
