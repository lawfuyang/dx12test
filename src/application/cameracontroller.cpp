#include "application/cameracontroller.h"

#include "graphic/gfxview.h"

void CameraController::Initialize()
{
    Reset();

    m_MouseLastPos = { Mouse::GetX(), Mouse::GetY() };
}

bbeMatrix CameraController::Get3DViewProjMatrix()
{
    const float aspectRatio = (float)g_CommandLineOptions.m_WindowWidth / (float)g_CommandLineOptions.m_WindowHeight; // TODO: retrieve window dimensions from somewhere legit
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
    m_EyePosition += finalMoveVector * m_CameraMoveSpeed;
}

void CameraController::UpdateCameraRotation()
{
    const bbeVector2 mousePos{ Mouse::GetX(), Mouse::GetY() };
    const bbeVector2 mouseDeltaVec = mousePos - m_MouseLastPos;

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

    // lock mouse to center of window when rotating camera
    // get engine window dimensions
    ::RECT clientRect = {};
    ::GetClientRect(g_System.GetEngineWindowHandle(), &clientRect);

    // get engine window position in monitor screen coords
    ::POINT clientToScreen = {};
    ::ClientToScreen(g_System.GetEngineWindowHandle(), &clientToScreen);

    // lock mouse to center of window, in monitor screen coords
    ::SetCursorPos(clientToScreen.x + (clientRect.right / 2), clientToScreen.y + (clientRect.bottom / 2));

    // set last mouse pos to center of window, in window coords
    m_MouseLastPos = { (float)g_CommandLineOptions.m_WindowWidth / 2, (float)g_CommandLineOptions.m_WindowHeight / 2 };
}

void CameraController::Update()
{
    UpdateEyePosition();

    if (Mouse::IsButtonPressed(Mouse::Right))
    {
        UpdateCameraRotation();
    }
    else
    {
        m_MouseLastPos = { Mouse::GetX(), Mouse::GetY() };
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
