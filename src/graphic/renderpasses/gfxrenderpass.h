#pragma once

class GfxContext;

class GfxRenderPass
{
public:
    GfxRenderPass(const char* name)
        : m_Name(name)
    {}

    virtual ~GfxRenderPass() {}

    virtual void Initialize(GfxContext&) = 0;
    virtual void ShutDown() = 0;
    virtual void Render(GfxContext&) = 0;

    const char* GetName() const { return m_Name; }

protected:
    const char* m_Name = "Unnamed Render Pass";
};
