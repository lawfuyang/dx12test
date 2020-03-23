#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const bbeVector2& f) { x = f.x; y = f.y; }                   \
        operator bbeVector2() const { return bbeVector2{x,y}; }               
                                                                          
#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const bbeVector4& f) { x = f.x; y = f.y; z = f.z; w = f.w; } \
        operator bbeVector4() const { return bbeVector4{x,y,z,w}; }

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
