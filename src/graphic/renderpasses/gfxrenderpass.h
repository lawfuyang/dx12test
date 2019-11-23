#pragma once

class GfxContext;

class GfxRenderPass
{
public:
    GfxRenderPass(const char* name)
        : m_Name(name)
    {}

    virtual ~GfxRenderPass() {}

    virtual void Render(const GfxContext&) = 0;

    const char* GetName() const { return m_Name; }

protected:
    const char* m_Name = nullptr;
};
