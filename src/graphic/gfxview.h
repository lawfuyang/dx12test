#pragma once

class GfxView
{
public:
    DeclareSingletonFunctions(GfxView);

private:
    float    m_FOV              = 45.0f; // in degrees
    float    m_Near             = 0.1f;
    float    m_Far              = 1000.0f;
    XMMATRIX m_ViewMatrix       = XMMatrixIdentity();
    XMMATRIX m_ProjectionMatrix = XMMatrixIdentity();
    XMMATRIX m_ViewProjMatrix   = XMMatrixIdentity();
};
#define g_GfxView GfxView::GetInstance()
