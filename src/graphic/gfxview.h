#pragma once

class GfxView
{
public:
    DeclareSingletonFunctions(GfxView);

    void SetViewProjMatrix(const bbeMatrix& mat) { m_ViewProjMatrix = mat; }
    const bbeMatrix& GetViewProjMatrix() const { return m_ViewProjMatrix; }

private:
    float     m_FOV            = 45.0f; // in degrees
    float     m_Near           = 0.1f;
    float     m_Far            = 1000.0f;
    bbeMatrix m_ViewProjMatrix = bbeMatrix::Identity;
};
#define g_GfxView GfxView::GetInstance()
