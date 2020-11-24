#include <graphic/view.h>
#include <graphic/pch.h>

void View::CreatePerspective(float newNear, float newFar, float newFOV)
{
    m_ZNearP = newNear;
    m_ZFarP = newFar;
    m_FOV = newFOV;

    Update();
}

void View::Update()
{
    const float aspectRatio = (float)g_CommandLineOptions.m_WindowWidth / (float)g_CommandLineOptions.m_WindowHeight;

    m_Projection = CreatePerspectiveFieldOfViewLH(m_FOV, aspectRatio, m_ZNearP, m_ZFarP);
    m_View = CreateLookAtLH(m_Eye, m_Eye + m_LookAt, m_Up);
    m_VP = m_Projection * m_View;

    m_InvView = m_View.Invert();
    m_InvProjection = m_Projection.Invert();
    m_InvVP = m_VP.Invert();

    m_GfxProjection = m_Projection.Transpose();
    m_GfxView = m_View.Transpose();
    m_GfxVP = m_GfxProjection * m_GfxView;

    m_GfxInvView = m_GfxProjection.Transpose();
    m_GfxInvProjection = m_GfxView.Transpose();
    m_GfxInvVP = m_GfxVP.Transpose();

    m_Frustum.Create(m_VP);
}
