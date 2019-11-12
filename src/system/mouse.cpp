#include "system/system.h"
#include "system/mouse.h"


void Mouse::ProcessMouseMove(uint32_t param, int32_t x, int32_t y)
{
    m_Pos[0] = (float)x;
    m_Pos[1] = (float)y;

    UpdateButton(Button::Left, (param & MK_LBUTTON) > 0);
    UpdateButton(Button::Right, (param & MK_RBUTTON) > 0);
}

void Mouse::ProcessMouseWheel(int32_t delta)
{
    m_Wheel += delta;
}

void Mouse::UpdateButton(Button button, bool pressed)
{
    const uint32_t buttonIdx = (int32_t)button;

    if (!pressed && m_Pressed[buttonIdx])
        m_WasReleased[buttonIdx] = true;
    if (pressed && !m_Pressed[buttonIdx])
        m_WasPressed[buttonIdx] = true;

    m_Pressed[buttonIdx] = pressed;
}

bool Mouse::IsButtonPressed(Button key)
{
    return Mouse::GetInstance().m_Pressed[(int32_t)key];
}

bool Mouse::WasButtonPressed(Button key)
{
    return Mouse::GetInstance().m_WasPressed[(int32_t)key];
}

bool Mouse::WasButtonReleased(Button key)
{
    return Mouse::GetInstance().m_WasReleased[(int32_t)key];
}

bool Mouse::WasHeldFor(Button key, float time)
{
    const uint32_t keyIdx = (int32_t)key;
    return (Mouse::GetInstance().m_PressedTime[keyIdx] >= time) && !Mouse::GetInstance().m_WasReleased[keyIdx];
}

bool Mouse::WasClicked(Button key, float time)
{
    const uint32_t keyIdx = (int32_t)key;
    return Mouse::GetInstance().m_WasReleased[keyIdx] && (Mouse::GetInstance().m_PressedTime[keyIdx] < time);
}

float Mouse::GetX()
{
    return Mouse::GetInstance().m_Pos[0];
}

float Mouse::GetY()
{
    return Mouse::GetInstance().m_Pos[1];
}

int32_t Mouse::GetWheel()
{
    return Mouse::GetInstance().m_Wheel;
}

void Mouse::Tick(float timeDelta)
{
    bbeMemZeroArray(m_WasPressed);
    bbeMemZeroArray(m_WasReleased);

    for (uint32_t i = 0; i < _countof(m_PressedTime); ++i)
    {
        if (m_Pressed[i])
            m_PressedTime[i] += timeDelta;
        else
            m_PressedTime[i] = 0;
    }
}
