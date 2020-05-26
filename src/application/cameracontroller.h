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

    template <typename Archive>
    void Serialize(Archive&);

    float m_Near = 1.0f;
    float m_Far = 2000.0f;
    float m_FOV = bbeRad45;;

    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_MouseRotationSpeed = 0.002f;
    float m_CameraMoveSpeed = 1.0f;

    bbeVector2 m_CurrentMousePos = {};
    bbeVector2 m_MouseLastPos = {};

    bbeVector3 m_EyePosition; // Where the camera is in world space. Z increases into of the screen when using LH coord system (which we are and DX uses)
    bbeVector3 m_Dir;         // Direction that the camera is looking at
    bbeVector3 m_UpDirection; // Which way is up
    bbeVector3 m_RightDirection;

    friend class cereal::access;
};
#define g_CameraController CameraController::GetInstance()
