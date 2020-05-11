#include <application/cameracontroller.h>

#include <system/imguimanager.h>
#include <graphic/gfxview.h>

void CameraController::Initialize()
{
    Reset();

    m_CurrentMousePos = m_MouseLastPos = { Mouse::GetX(), Mouse::GetY() };

    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUIPropertyGrid(); });
}

bbeMatrix CameraController::Get3DViewProjMatrix()
{
    const float aspectRatio = (float)g_CommandLineOptions.m_WindowWidth / (float)g_CommandLineOptions.m_WindowHeight;
    const float fovAngleY = DirectX::XMConvertToRadians(m_FOV);

    const bbeVector3 focusPosition = m_EyePosition + m_Dir;
    const bbeMatrix viewMatrix = CreateLookAtLH(m_EyePosition, focusPosition, m_UpDirection).Transpose();
    const bbeMatrix projMatrix = CreatePerspectiveFieldOfViewLH(fovAngleY, aspectRatio, m_Near, m_Far).Transpose();

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

    ImGui::Begin("CameraController");

    ImGui::InputFloat3("Position", (float*)&m_EyePosition);
    ImGui::InputFloat3("Direction", (float*)&m_Dir, "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Right", (float*)&m_RightDirection, "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Up", (float*)&m_UpDirection, "%.3f", ImGuiInputTextFlags_ReadOnly);
    doUpdate |= ImGui::InputFloat("Yaw", &m_Yaw);
    doUpdate |= ImGui::InputFloat("Pitch", &m_Pitch);

    ImGui::NewLine();
    ImGui::InputFloat("Move Speed", &m_CameraMoveSpeed);
    ImGui::InputFloat("Rotate Speed", &m_MouseRotationSpeed);

    ImGui::NewLine();
    doReset = ImGui::Button("Reset");

    ImGui::End();

    if (doUpdate)
    {
        UpdateCameraRotation();
    }

    if (doReset)
    {
        Reset();
    }
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
    m_EyePosition = { 0.0f, 5.0f, -10.0f };
    m_Pitch = -bbeRad30;
    m_Yaw = 0.0f;
    m_MouseRotationSpeed = 0.002f;
    m_CameraMoveSpeed = 0.01f;

    UpdateCameraRotation();
}
