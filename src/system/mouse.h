#pragma once

class Mouse
{
    DeclareSingletonFunctions(Mouse);

    enum Button
    {
        Left = 1,
        Right = 2,
        ButtonCount = 3,
    };

public:
    void ProcessMouseMove(uint32_t param, int32_t x, int32_t y);
    void ProcessMouseWheel(int32_t delta);
    void UpdateButton(Button button, bool pressed);

    static bool IsButtonPressed(Button key);
    static bool WasButtonPressed(Button key);
    static bool WasButtonReleased(Button key);
    static bool WasHeldFor(Button key, float time);
    static bool WasClicked(Button key, float time);

    // From [0, Resolution.X]
    static float GetX();

    // From [0, Resolution.Y]
    static float GetY();

    static int32_t GetWheel();

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
