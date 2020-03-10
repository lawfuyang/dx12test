#pragma once

class CameraController
{
public:
    DeclareSingletonFunctions(CameraController);

    void Initialize();
    void Update();

private:
    void Reset();
    XMMATRIX CameraController::Get3DViewProjMatrix();

    float m_Near = 0.01f;
    float m_Far  = 100.0f;
    float m_FOV  = 90.0f;

    XMVECTOR m_EyePosition; // Where the camera is in world space. Z increases into of the screen when using LH coord system (which we are and DX uses)
    XMVECTOR m_Dir;         // Direction that the camera is looking at
    XMVECTOR m_UpDirection; // Which way is up
};
#define g_CameraController CameraController::GetInstance()
