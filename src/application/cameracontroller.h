#pragma once

class CameraController
{
public:
    DeclareSingletonFunctions(CameraController);

    void Initialize();
    void Update();

private:
    void Reset();
    bbeMatrix CameraController::Get3DViewProjMatrix();
    void UpdateEyePosition();
    void UpdateCameraRotation();
    void UpdateIMGUIPropertyGrid();

    float m_Near = 0.01f;
    float m_Far  = 100.0f;
    float m_FOV  = 90.0f;

    float m_VerticalAngle = 0.0f;
    float m_HorizontalAngle = 0.0f;
    float m_MouseRotationSpeed = 0.002f;
    float m_CameraMoveSpeed = 0.01f;

    bbeVector2 m_CurrentMousePos = {};
    bbeVector2 m_MouseLastPos = {};

    bbeVector3 m_EyePosition; // Where the camera is in world space. Z increases into of the screen when using LH coord system (which we are and DX uses)
    bbeVector3 m_Dir;         // Direction that the camera is looking at
    bbeVector3 m_UpDirection; // Which way is up
    bbeVector3 m_RightDirection;
};
#define g_CameraController CameraController::GetInstance()
