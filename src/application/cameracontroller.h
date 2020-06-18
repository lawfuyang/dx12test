#pragma once

class CameraController
{
public:
    DeclareSingletonFunctions(CameraController);

    void Initialize();
    void Update();

private:
    void Reset();
    void UpdateEyePosition();
    void UpdateCameraRotation();
    void UpdateIMGUIPropertyGrid();

    template <typename Archive>
    void Serialize(Archive&);

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_MouseRotationSpeed = 0.002f;
    float m_CameraMoveSpeed = 1.0f;

    bbeVector2 m_CurrentMousePos;
    bbeVector2 m_MouseLastPos;

    bbeVector3 m_EyePosition; // Where the camera is in world space. Z increases into of the screen when using LH coord system (which we are and DX uses)

    friend class cereal::access;
};
#define g_CameraController CameraController::GetInstance()
