#pragma once

class CameraController
{
public:
    DeclareSingletonFunctions(CameraController);

    void Initialize();
    void Update();

private:
    void Reset();
    Matrix CameraController::Get3DViewProjMatrix();
    void UpdateEyePosition();
    void UpdateCameraRotation();

    float m_Near = 0.01f;
    float m_Far  = 100.0f;
    float m_FOV  = 90.0f;

    float m_VerticalAngle = 0.0f;
    float m_HorizontalAngle = 0.0f;
    float m_MouseRotationSpeed = 0.002f;
    float m_CameraMoveSpeed = 0.1f;
    Vector2 m_MouseLastPos = {};

    Vector3 m_EyePosition; // Where the camera is in world space. Z increases into of the screen when using LH coord system (which we are and DX uses)
    Vector3 m_Dir;         // Direction that the camera is looking at
    Vector3 m_UpDirection; // Which way is up
    Vector3 m_RightDirection;
};
#define g_CameraController CameraController::GetInstance()
