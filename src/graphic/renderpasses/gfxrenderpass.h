#pragma once

class GfxContext;

class GfxRenderPass
{
public:
    virtual void Initialize() = 0;
    virtual void ShutDown() = 0;
    virtual void PopulateCommandList(GfxContext&) = 0;

    const char* GetName() const { return m_Name; }
    GfxContext* GetGfxContext() const { return m_Context; }

protected:
    const char* m_Name = "Unnamed Render Pass";
    GfxContext* m_Context = nullptr;
};
