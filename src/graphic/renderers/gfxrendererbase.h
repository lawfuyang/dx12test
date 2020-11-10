#pragma once

class GfxContext;

class GfxRendererBase
{
public:
    virtual void Initialize() = 0;
    virtual void ShutDown() = 0;
    virtual void PopulateCommandList(GfxContext& context) = 0;
    virtual const char* GetName() const = 0;
};
