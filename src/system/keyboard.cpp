#include "system/system.h"
#include "system/keyboard.h"

bool Keyboard::IsKeyPressed(Key key)
{
    return GetInstance().m_Pressed[(uint32_t)key];
}

bool Keyboard::WasKeyReleased(Key key)
{
    return GetInstance().m_WasReleased[(uint32_t)key];
}

bool Keyboard::WasKeyPressed(Key key)
{
    return GetInstance().m_WasPressed[(uint32_t)key];
}

void Keyboard::Tick()
{
    bbeMemZeroArray(m_WasPressed);
    bbeMemZeroArray(m_WasReleased);
}

void Keyboard::ProcessKeyUp(uint32_t wParam)
{
    m_Pressed[wParam] = false;
    m_WasReleased[wParam] = true;
}

void Keyboard::ProcessKeyDown(uint32_t wParam)
{
    m_Pressed[wParam] = true;
    m_WasPressed[wParam] = true;
}
