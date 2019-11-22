#pragma once

class GfxDevice;

class GfxRenderPass
{
public:
    GfxRenderPass(const char* name)
        : m_Name(name)
    {}

    virtual ~GfxRenderPass() {}

    virtual void Render(GfxDevice&) = 0;

    const char* GetName() const { return m_Name; }

protected:
    const char* m_Name = nullptr;
};
