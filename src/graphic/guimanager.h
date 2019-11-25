#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

// TODO: implement imgui vec to dx math
/*
#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const MyVec2& f) { x = f.x; y = f.y; }                       \
        operator MyVec2() const { return MyVec2(x,y); }

#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const MyVec4& f) { x = f.x; y = f.y; z = f.z; w = f.w; }     \
        operator MyVec4() const { return MyVec4(x,y,z,w); }
*/

#include "extern/imgui/imgui.h"

class GUIManager
{
public:
    void Initialize();
    void ShutDown();

    void HandleWindowsInput(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void BeginFrame();
    void EndFrameAndRenderGUI();

private:

};
