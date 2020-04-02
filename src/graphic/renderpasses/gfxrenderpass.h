#pragma once

class GfxContext;

class GfxRenderPass
{
public:
    virtual ~GfxRenderPass() {}

    virtual void Initialize() = 0;
    virtual void ShutDown() = 0;
    virtual void Render(GfxContext&) = 0;

    const char* GetName() const { return m_Name; }

protected:
    const char* m_Name = "Unnamed Render Pass";
};
