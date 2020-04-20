#include <application/cameracontroller.h>

#include <system/imguimanager.h>
#include <graphic/gfxview.h>

void CameraController::Initialize()
{
    Reset();

    m_CurrentMousePos = m_MouseLastPos = { Mouse::GetX(), Mouse::GetY() };

    g_System.AddSystemCommand([&]() 
        {
            g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUIPropertyGrid(); });
        });
}

bbeMatrix CameraController::Get3DViewProjMatrix()
{
    const float aspectRatio = (float)g_CommandLineOptions.m_WindowWidth / (float)g_CommandLineOptions.m_WindowHeight;
    const float fovAngleY = DirectX::XMConvertToRadians(m_FOV);

    const bbeVector3 focusPosition = m_EyePosition + m_Dir;
    const bbeMatrix viewMatrix = bbeMatrix::CreateLookAtLH(m_EyePosition, focusPosition, m_UpDirection).Transpose();
    const bbeMatrix projMatrix = bbeMatrix::CreatePerspectiveFieldOfViewLH(fovAngleY, aspectRatio, m_Near, m_Far).Transpose();

    return projMatrix * viewMatrix;
}

void CameraController::UpdateEyePosition()
{
    bbeVector3 finalMoveVector = bbeVector3::Zero;

    if (Keyboard::IsKeyPressed(Keyboard::KEY_A))
    {
        finalMoveVector -= m_RightDirection;
    }
    if (Keyboard::IsKeyPressed(Keyboard::KEY_D))
    {
        finalMoveVector += m_RightDirection;
    }
    if (Keyboard::IsKeyPressed(Keyboard::KEY_W))
    {
        finalMoveVector += m_Dir;
    }
    if (Keyboard::IsKeyPressed(Keyboard::KEY_S))
    {
        finalMoveVector -= m_Dir;
    }

    finalMoveVector.Normalize();
    m_EyePosition += finalMoveVector * m_CameraMoveSpeed * (float)g_System.GetFrameTimeMs();
}

void CameraController::UpdateCameraRotation()
{
    const bbeVector2 mouseDeltaVec = m_CurrentMousePos - m_MouseLastPos;

    // compute new camera angles and vectors based off mouse delta
    m_HorizontalAngle -= m_MouseRotationSpeed * float(mouseDeltaVec.x);
    m_VerticalAngle += m_MouseRotationSpeed * float(mouseDeltaVec.y);

    m_Dir =
    {
        std::cos(m_VerticalAngle) * std::sin(m_HorizontalAngle),
        std::sin(m_VerticalAngle),
        std::cos(m_VerticalAngle) * std::cos(m_HorizontalAngle),
    };

    m_RightDirection =
    {
        std::sin(m_HorizontalAngle - bbePIBy2),
        0,
        std::cos(m_HorizontalAngle - bbePIBy2),
    };
}

void CameraController::UpdateIMGUIPropertyGrid()
{
    ImGui::Begin("CameraController");
    ImGui::Text("Current Mouse Position: %.3f, %.3f", m_CurrentMousePos.x, m_CurrentMousePos.y);
    ImGui::End();
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

    m_UpDirection = m_RightDirection.Cross(m_EyePosition);

    const bbeMatrix viewProjMatrix = Get3DViewProjMatrix();
    g_GfxView.SetViewProjMatrix(viewProjMatrix);
}

void CameraController::Reset()
{
    m_EyePosition = { 0.0f, 0.0f, -1.0f };
    m_Dir = { 0.0f, 0.0f, 1.0f };
    m_UpDirection = { 0.0f, 1.0f, 0.0f };
    m_RightDirection = { 1.0f, 0.0f, 0.0f };
}
