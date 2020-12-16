#include <application/cameracontroller.h>

#include <system/imguimanager.h>

#include <graphic/gfx/gfxmanager.h>

ForwardDeclareSerializerFunctions(CameraController);

static bool gs_ShowCameraControllerIMGUIWindow = false;

void CameraController::Initialize()
{
    Reset();

    m_CurrentMousePos = m_MouseLastPos = { g_Mouse.GetX(), g_Mouse.GetY() };

    g_IMGUIManager.RegisterTopMenu("Graphic", "Camera Controller", &gs_ShowCameraControllerIMGUIWindow);
    g_IMGUIManager.RegisterWindowUpdateCB([&]() { UpdateIMGUIPropertyGrid(); });
}

void CameraController::UpdateEyePosition()
{
    View& view = g_GfxManager.GetMainView();

    // Calculate the move vector in camera space.
    bbeVector3 finalMoveVector;

    if (g_Keyboard.IsKeyPressed(Keyboard::KEY_A))
    {
        finalMoveVector += view.m_Right;
    }
    if (g_Keyboard.IsKeyPressed(Keyboard::KEY_D))
    {
        finalMoveVector -= view.m_Right;
    }
    if (g_Keyboard.IsKeyPressed(Keyboard::KEY_W))
    {
        finalMoveVector += view.m_LookAt;
    }
    if (g_Keyboard.IsKeyPressed(Keyboard::KEY_S))
    {
        finalMoveVector -= view.m_LookAt;
    }

    if (finalMoveVector.LengthSquared() > 0.1f)
    {
        finalMoveVector.Normalize();
        view.m_Eye += finalMoveVector * m_CameraMoveSpeed * (float)g_System.GetFrameTimeMs();
        view.Update();
        m_EyePosition = view.m_Eye;
    }
}

void CameraController::UpdateCameraRotation()
{
    View& view = g_GfxManager.GetMainView();

    const bbeVector2 mouseDeltaVec = m_CurrentMousePos - m_MouseLastPos;

    // compute new camera angles and vectors based off mouse delta
    m_Yaw += m_MouseRotationSpeed * float(mouseDeltaVec.x);
    m_Pitch -= m_MouseRotationSpeed * float(mouseDeltaVec.y);

    // Prevent looking too far up or down.
    m_Pitch = std::clamp(m_Pitch, -bbePIBy2, bbePIBy2);

    const float r = std::cos(m_Pitch);
    view.m_LookAt =
    {
        r * std::sin(m_Yaw),
        std::sin(m_Pitch),
        r * std::cos(m_Yaw),
    };

    view.m_Right =
    {
        std::sin(m_Yaw - bbePIBy2),
        0,
        std::cos(m_Yaw - bbePIBy2),
    };

    view.Update();
}

void CameraController::UpdateIMGUIPropertyGrid()
{
    if (!gs_ShowCameraControllerIMGUIWindow)
        return;

    View& view = g_GfxManager.GetMainView();

    bool doUpdate = false;
    bool doReset = false;
    bool doSave = false;

    ScopedIMGUIWindow window{ "CameraController" };

    doUpdate |= ImGui::InputFloat3("Position", (float*)&m_EyePosition);
    ImGui::InputFloat3("Direction", (float*)&view.m_LookAt, "%.3f", ImGuiInputTextFlags_ReadOnly);
    doUpdate |= ImGui::SliderAngle("Yaw", &m_Yaw, -180.0f, 180.0f);
    doUpdate |= ImGui::SliderAngle("Pitch", &m_Pitch, -90.0f, 90.0f);
    doUpdate |= ImGui::SliderFloat("Far", &view.m_ZFarP, 100.0f, 10000.0f);
    doUpdate |= ImGui::SliderAngle("FOV", &view.m_FOV, 1.0f, 179.0f);

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
    m_CurrentMousePos = { g_Mouse.GetX(), g_Mouse.GetY() };

    // for some weird reason, windows underflows to 65535 when cursor is crosses left/top window
    m_CurrentMousePos.x = m_CurrentMousePos.x > 60000.0f ? 0.0f : m_CurrentMousePos.x;
    m_CurrentMousePos.y = m_CurrentMousePos.y > 60000.0f ? 0.0f : m_CurrentMousePos.y;
    m_CurrentMousePos.Clamp(bbeVector2::Zero, { (float)g_CommandLineOptions.m_WindowWidth, (float)g_CommandLineOptions.m_WindowHeight });

    UpdateEyePosition();

    if (g_Mouse.IsButtonPressed(g_Mouse.Right))
    {
        UpdateCameraRotation();
    }
}

void CameraController::Reset()
{
    if (g_Serializer.Read(Serializer::JSON, "CameraControllerValues", *this))
    {
        View& view = g_GfxManager.GetMainView();
        view.m_Eye = m_EyePosition;

        UpdateCameraRotation();
    }
}
