#include <application/cameracontroller.h>

#include <system/imguimanager.h>
#include <graphic/gfxview.h>

void CameraController::Initialize()
{
    Reset();

    g_Serializer.Read(Serializer::JSON, "CameraControllerValues", *this);
    UpdateCameraRotation();

    m_CurrentMousePos = m_MouseLastPos = { Mouse::GetX(), Mouse::GetY() };

    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUIPropertyGrid(); });
}

bbeMatrix CameraController::Get3DViewProjMatrix()
{
    const float aspectRatio = (float)g_CommandLineOptions.m_WindowWidth / (float)g_CommandLineOptions.m_WindowHeight;

    const bbeVector3 focusPosition = m_EyePosition + m_Dir;
    const bbeMatrix viewMatrix = CreateLookAtLH(m_EyePosition, focusPosition, m_UpDirection).Transpose();
    const bbeMatrix projMatrix = CreatePerspectiveFieldOfViewLH(m_FOV, aspectRatio, m_Near, m_Far).Transpose();

    return projMatrix * viewMatrix;
}

void CameraController::UpdateEyePosition()
{
    // Calculate the move vector in camera space.
    bbeVector3 finalMoveVector;

    if (Keyboard::IsKeyPressed(Keyboard::KEY_A))
    {
        finalMoveVector += m_RightDirection;
    }
    if (Keyboard::IsKeyPressed(Keyboard::KEY_D))
    {
        finalMoveVector -= m_RightDirection;
    }
    if (Keyboard::IsKeyPressed(Keyboard::KEY_W))
    {
        finalMoveVector += m_Dir;
    }
    if (Keyboard::IsKeyPressed(Keyboard::KEY_S))
    {
        finalMoveVector -= m_Dir;
    }

    if (finalMoveVector.LengthSquared() > 0.1f)
    {
        finalMoveVector.Normalize();
    }

    m_EyePosition += finalMoveVector * m_CameraMoveSpeed * (float)g_System.GetFrameTimeMs();
}

void CameraController::UpdateCameraRotation()
{
    const bbeVector2 mouseDeltaVec = m_CurrentMousePos - m_MouseLastPos;

    // compute new camera angles and vectors based off mouse delta
    m_Yaw += m_MouseRotationSpeed * float(mouseDeltaVec.x);
    m_Pitch -= m_MouseRotationSpeed * float(mouseDeltaVec.y);

    // Prevent looking too far up or down.
    m_Pitch = std::clamp(m_Pitch, -bbePIBy2, bbePIBy2);

    const float r = std::cos(m_Pitch);
    m_Dir =
    {
        r * std::sin(m_Yaw),
        std::sin(m_Pitch),
        r * std::cos(m_Yaw),
    };

    m_RightDirection =
    {
        std::sin(m_Yaw - bbePIBy2),
        0,
        std::cos(m_Yaw - bbePIBy2),
    };
}

void CameraController::UpdateIMGUIPropertyGrid()
{
    bool doUpdate = false;
    bool doReset = false;
    bool doSave = false;

    ScopedIMGUIWindow window{ "CameraController" };

    ImGui::InputFloat3("Position", (float*)&m_EyePosition);
    ImGui::InputFloat3("Direction", (float*)&m_Dir, "%.3f", ImGuiInputTextFlags_ReadOnly);
    doUpdate |= ImGui::SliderAngle("Yaw", &m_Yaw, -180.0f, 180.0f);
    doUpdate |= ImGui::SliderAngle("Pitch", &m_Pitch, -90.0f, 90.0f);
    ImGui::SliderFloat("Far", &m_Far, 100.0f, 10000.0f);
    ImGui::SliderAngle("FOV", &m_FOV, 1.0f, 179.0f);

    ImGui::NewLine();
    ImGui::InputFloat("Move Speed", &m_CameraMoveSpeed);
    ImGui::InputFloat("Rotate Speed", &m_MouseRotationSpeed);

    ImGui::NewLine();
    doReset = ImGui::Button("Reset");

    ImGui::SameLine();
    doSave = ImGui::Button("Save Values");

    if (doUpdate)
    {
        UpdateCameraRotation();
    }

    if (doReset)
    {
        Reset();
    }

    if (doSave)
    {
        g_Serializer.Write(Serializer::JSON, "CameraControllerValues", *this);
    }
}

template<typename Archive>
void CameraController::Serialize(Archive& ar)
{
    ar(CEREAL_NVP(m_Yaw));
    ar(CEREAL_NVP(m_Pitch));
    ar(CEREAL_NVP(m_MouseRotationSpeed));
    ar(CEREAL_NVP(m_CameraMoveSpeed));
    ar(CEREAL_NVP(m_EyePosition));
}

void CameraController::Update()
{
    m_MouseLastPos = m_CurrentMousePos;
    m_CurrentMousePos = { Mouse::GetX(), Mouse::GetY() };

    // for some weird reason, windows underflows to 65535 when cursor is crosses left/top window
    m_CurrentMousePos.x = m_CurrentMousePos.x > 60000.0f ? 0.0f : m_CurrentMousePos.x;
    m_CurrentMousePos.y = m_CurrentMousePos.y > 60000.0f ? 0.0f : m_CurrentMousePos.y;
    m_CurrentMousePos.Clamp(bbeVector2::Zero, { (float)g_CommandLineOptions.m_WindowWidth, (float)g_CommandLineOptions.m_WindowHeight });

    UpdateEyePosition();

    if (Mouse::IsButtonPressed(Mouse::Right))
    {
        UpdateCameraRotation();
    }

    m_UpDirection = m_RightDirection.Cross(m_Dir);

    const bbeMatrix viewProjMatrix = Get3DViewProjMatrix();
    g_GfxView.SetViewProjMatrix(viewProjMatrix);
}

void CameraController::Reset()
{
    m_EyePosition = { 0.0f, 200.0f, -400.0f };
    m_Pitch = -bbeRad15;
    m_Yaw = 0.0f;
    m_MouseRotationSpeed = 0.002f;
    m_CameraMoveSpeed = 1.0f;

    m_Near = 1.0f;
    m_Far = 2000.0f;
    m_FOV = bbeRad45;

    UpdateCameraRotation();
}
