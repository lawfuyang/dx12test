#pragma once

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#define IM_VEC2_CLASS_EXTRA                                                 \
        ImVec2(const bbeVector2& f) { x = f.x; y = f.y; }                   \
        operator bbeVector2() const { return bbeVector2{x,y}; }               
                                                                          
#define IM_VEC4_CLASS_EXTRA                                                 \
        ImVec4(const bbeVector4& f) { x = f.x; y = f.y; z = f.z; w = f.w; } \
        operator bbeVector4() const { return bbeVector4{x,y,z,w}; }

#include <extern/imgui/imgui.h>

struct IMGUICmdList
{
    std::vector<ImDrawVert> m_VB;
    std::vector<ImDrawIdx> m_IB;
    std::vector<ImDrawCmd> m_DrawCmd;
};

struct IMGUIDrawData
{
    std::vector<IMGUICmdList> m_DrawList;
    uint32_t                  m_VtxCount = 0;
    uint32_t                  m_IdxCount = 0;
    bbeVector2                m_Pos      = bbeVector2::Zero;
    bbeVector2                m_Size     = bbeVector2::Zero;
};

class IMGUIManager
{
public:
    DeclareSingletonFunctions(IMGUIManager);

    using FileDialogResultFinalizer = std::function<void(const std::string&, const std::string&)>;

    void Initialize();
    void ShutDown();

    void ProcessWindowsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void Update();
    void RegisterWindowUpdateCB(const std::function<void()>&);
    void RegisterGeneralButtonCB(const std::function<void()>&, bool* triggerBool);
    void RegisterTopMenu(const std::string& mainCategory, const std::string& buttonName, bool* windowToggle = nullptr);
    void RegisterFileDialog(const std::string& vName, const char* vFilters, const FileDialogResultFinalizer&);

private:
    void SaveDrawData();
    IMGUIDrawData GetDrawData() const { return m_DrawData[m_DrawDataIdx]; }

    bool m_ShowDemoWindow = false;

    std::mutex m_WindowCBsLock;
    std::vector<std::function<void()>> m_UpdateCBs;
    std::unordered_map<bool*, std::function<void()>> m_TriggerCBs;
    std::map<std::string, std::vector<std::pair<std::string, bool*>>> m_TopMenusCBs; // not unordered_map, because i want the menu buttons to be sorted alphabetically
    std::pair<std::string, FileDialogResultFinalizer> m_ActiveFileDialog;

    Timer m_Timer;

    IMGUIDrawData m_DrawData[2];
    uint32_t m_DrawDataIdx = 0;

    friend class cereal::access;
    friend class GfxIMGUIRenderer;
};
#define g_IMGUIManager IMGUIManager::GetInstance()

// Helper scoped class for always auto-resized imgui windows
struct ScopedIMGUIWindow
{
    ScopedIMGUIWindow(const char* windowName);
    ~ScopedIMGUIWindow();
};
