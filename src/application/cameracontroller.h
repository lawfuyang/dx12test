#pragma once

class CameraController
{
public:
    DeclareSingletonFunctions(CameraController);

    void Initialize();
    void Update();

    void Get3DViewProjMatrices(XMFLOAT4X4& view, XMFLOAT4X4& proj, float fovInDegrees, float aspectRatio);
    void Set(XMVECTOR eye, XMVECTOR at, XMVECTOR up);
    void RotateYaw(float deg);
    void RotatePitch(float deg);

private:
    XMVECTOR m_Eye; // Where the camera is in world space. Z increases into of the screen when using LH coord system (which we are and DX uses)
    XMVECTOR m_At;  // What the camera is looking at (world origin)
    XMVECTOR m_Up;  // Which way is up
};
#define g_CameraController CameraController::GetInstance()
