#pragma once

class GfxContext;

class GfxRenderPass
{
public:
    GfxRenderPass(const char* name)
        : m_Name(name)
    {}

    virtual ~GfxRenderPass() { g_Log.info("Destroying Render Pass: '{}'", m_Name); }

    virtual void Render(GfxContext&) = 0;

    const char* GetName() const { return m_Name; }

protected:
    const char* m_Name = "Unnamed Render Pass";
};
