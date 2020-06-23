#pragma once

class GfxContext;

class GfxRendererBase
{
public:
    virtual void Initialize() = 0;
    virtual void ShutDown() = 0;
    virtual void PopulateCommandList() = 0;
    virtual const char* GetName() const = 0;

    GfxContext* GetGfxContext() const { return m_Context; }

protected:
    GfxContext* m_Context = nullptr;
};
