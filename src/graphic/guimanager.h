#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#define IM_VEC2_CLASS_EXTRA                                              \
        ImVec2(const Vector2& f) { x = f.x; y = f.y; }                   \
        operator Vector2() const { return Vector2{x,y}; }               
                                                                          
#define IM_VEC4_CLASS_EXTRA                                              \
        ImVec4(const Vector4& f) { x = f.x; y = f.y; z = f.z; w = f.w; } \
        operator Vector4() const { return Vector4{x,y,z,w}; }

#include "extern/imgui/imgui.h"

class GUIManager
{
public:
    DeclareSingletonFunctions(GUIManager);

    void Initialize();
    void ShutDown();

    void HandleWindowsInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void BeginFrame();
    void EndFrameAndRenderGUI();

private:
};
