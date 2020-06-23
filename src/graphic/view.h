#pragma once

#include <graphic/shapes.h>

class View
{
public:
    void CreatePerspective(float newNear, float newFar, float newFOV);
    void Update();

    float m_ZNearP = 1.0f;
    float m_ZFarP  = 2000.0f;
    float m_FOV    = bbeRad45;

    bbeVector3 m_Eye    = bbeVector3::Zero;
    bbeVector3 m_LookAt = bbeVector3::UnitZ;
    bbeVector3 m_Up     = bbeVector3::UnitY;
    bbeVector3 m_Right  = bbeVector3::UnitX;

    bbeMatrix m_View           = bbeMatrix::Identity;
    bbeMatrix m_Projection     = bbeMatrix::Identity;
    bbeMatrix m_VP             = bbeMatrix::Identity;
    bbeMatrix m_InvView        = bbeMatrix::Identity;
    bbeMatrix m_InvProjection  = bbeMatrix::Identity;
    bbeMatrix m_InvVP          = bbeMatrix::Identity;

    bbeMatrix m_GfxView          = bbeMatrix::Identity;
    bbeMatrix m_GfxProjection    = bbeMatrix::Identity;
    bbeMatrix m_GfxVP            = bbeMatrix::Identity;
    bbeMatrix m_GfxInvView       = bbeMatrix::Identity;
    bbeMatrix m_GfxInvProjection = bbeMatrix::Identity;
    bbeMatrix m_GfxInvVP         = bbeMatrix::Identity;

    Frustum m_Frustum;
};
