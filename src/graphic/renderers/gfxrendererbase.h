#pragma once

class GfxContext;

class GfxRendererBase
{
public:
    virtual void Initialize() {};
    virtual void ShutDown() {};
    virtual void PopulateCommandList(GfxContext& context) {};
    virtual const char* GetName() const = 0;
};
