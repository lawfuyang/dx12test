#include "application/cameracontroller.h"

#include "graphic/gfxview.h"

void CameraController::Initialize()
{
    Reset();
}

XMMATRIX CameraController::Get3DViewProjMatrix()
{
    const float aspectRatio = (float)g_CommandLineOptions.m_WindowWidth / (float)g_CommandLineOptions.m_WindowHeight; // TODO: retrieve window dimensions from somewhere legit
    const float fovAngleY = XMConvertToRadians(m_FOV);

    const XMVECTOR focusPosition = m_EyePosition + m_Dir;
    const XMMATRIX viewMatrix = XMMatrixTranspose(XMMatrixLookAtLH(m_EyePosition, focusPosition, m_UpDirection));
    const XMMATRIX projMatrix = XMMatrixTranspose(XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, m_Near, m_Far));

    return projMatrix * viewMatrix;
}

void CameraController::Update()
{
    if (Keyboard::IsKeyPressed(Keyboard::KEY_A))
    {
        m_EyePosition += { -0.1f, 0.0f, 0.0f, 0.0f };
    }
    else if (Keyboard::IsKeyPressed(Keyboard::KEY_D))
    {
        m_EyePosition += { 0.1f, 0.0f, 0.0f, 0.0f };
    }
    else if (Keyboard::IsKeyPressed(Keyboard::KEY_W))
    {
        m_EyePosition += { 0.0f, 0.0f, 0.1f, 0.0f };
    }
    else if (Keyboard::IsKeyPressed(Keyboard::KEY_S))
    {
        m_EyePosition += { 0.0f, 0.0f, -0.1f, 0.0f };
    }

    // lock mouse to center of window when rotating camera
    if (Mouse::IsButtonPressed(Mouse::Right))
    {
        // get engine window dimensions
        ::RECT clientRect = {};
        ::GetClientRect(g_System.GetEngineWindowHandle(), &clientRect);

        // get engine window position in monitor screen coords
        ::POINT clientToScreen = {};
        ::ClientToScreen(g_System.GetEngineWindowHandle(), &clientToScreen);
        
        // lock mouse to center of window
        ::SetCursorPos(clientToScreen.x + (clientRect.right / 2), clientToScreen.y + (clientRect.bottom / 2));
    }

    const XMMATRIX viewProjMatrix = Get3DViewProjMatrix();
    g_GfxView.SetViewProjMatrix(viewProjMatrix);
}

void CameraController::Reset()
{
    m_EyePosition = { 0.0f, 0.0f, -1.0f, 1.0f };
    m_Dir = { 0.0f, 0.0f, 1.0f, 0.0f };
    m_UpDirection = { 0.0f, 1.0f, 0.0f, 0.0f };
}
