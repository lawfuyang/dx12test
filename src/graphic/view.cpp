#include "graphic/view.h"

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

    m_Projection = CreatePerspectiveFieldOfViewLH(m_FOV, aspectRatio, m_ZNearP, m_ZFarP).Transpose();

    m_View = CreateLookAtLH(m_Eye, m_Eye + m_LookAt, m_Up).Transpose();

    m_InvView = m_View.Invert();
    m_InvProjection = m_Projection.Invert();
    m_InvVP = m_VP.Invert();
    m_VP = m_Projection * m_View;
    m_Frustum.Create(m_VP);
}
