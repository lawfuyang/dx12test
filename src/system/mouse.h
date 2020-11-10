#pragma once

class Mouse
{
    DeclareSingletonFunctions(Mouse);

public:
    enum Button
    {
        Left = 1,
        Right = 2,
        ButtonCount = 3,
    };

    void ProcessMouseMove(uint32_t param, int32_t x, int32_t y);
    void ProcessMouseWheel(int32_t delta);
    void UpdateButton(Button button, bool pressed);

    bool IsButtonPressed(Button key);
    bool WasButtonPressed(Button key);
    bool WasButtonReleased(Button key);
    bool WasHeldFor(Button key, float time);
    bool WasClicked(Button key, float time);

    // From [0, Resolution.X]
    float GetX();

    // From [0, Resolution.Y]
    float GetY();

    int32_t GetWheel();

    void Tick();

private:
    bool    m_Pressed[ButtonCount]     = {};
    bool    m_WasPressed[ButtonCount]  = {};
    bool    m_WasReleased[ButtonCount] = {};
    float   m_PressedTime[ButtonCount] = {};
    float   m_Pos[2]                   = {};
    int32_t m_Wheel                    = 0;
};
#define g_Mouse Mouse::GetInstance()
