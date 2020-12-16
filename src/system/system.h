#pragma once

#include <system/commandmanager.h>

class System
{
    DeclareSingletonFunctions(System);

public:
    void Initialize();
    void Shutdown();
    void ProcessWindowsMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Loop();

    template <typename Lambda>
    void AddSystemCommand(Lambda&& lambda) { m_SystemCommandManager.AddCommand(std::forward<Lambda>(lambda)); }

    template <typename Lambda>
    void AddBGAsyncCommand(Lambda&& lambda) { m_BGAsyncCommandManager.AddCommand(std::forward<Lambda>(lambda)); }

    double GetFrameTimeMs() { return m_FrameTimeMs; }
    double GetFPS()         { return m_FPS; }

    uint32_t GetSystemFrameNumber() { return m_SystemFrameNumber; }

    tf::Executor& GetTasksExecutor() { return m_Executor; }

    ::HWND GetEngineWindowHandle() const { return m_EngineWindowHandle; }
    void SetEngineWindowHandle(::HWND handle) { m_EngineWindowHandle = handle; }

private:
    void RunKeyboardCommands();
    void BGAsyncThreadLoop();

    CommandManager m_SystemCommandManager;
    CommandManager m_BGAsyncCommandManager;

    ::HWND m_EngineWindowHandle = nullptr;

    bool m_Exit = false;
    bool m_BGAsyncThreadExit = false;

    double m_FPS = 0.0;
    double m_FrameTimeMs = 0.0;

    tf::Executor m_Executor;
    std::thread m_BGAsyncThread;

    Timer m_FrameTimer;
    uint32_t m_SystemFrameNumber = 0;
};
#define g_System System::GetInstance()

struct CommandLineOptions
{
    DeclareSingletonFunctions(CommandLineOptions);

    void Parse();

    uint32_t m_FPSLimit        = 200;
    bool     m_PIXCapture      = false;
    bool     m_ProfileInit     = false;
    bool     m_ProfileShutdown = false;
    uint32_t m_WindowWidth     = 1600;
    uint32_t m_WindowHeight    = 900;

    struct GfxDebugLayer
    {
        bool m_Enabled                                 = false;
        bool m_BreakOnWarnings                         = false;
        bool m_BreakOnErrors                           = false;
        bool m_EnableGPUValidation                     = false;
        bool m_EnableGPUResourceValidation             = false;
        bool m_EnableConservativeResourceStateTracking = false;
        bool m_BehaviorChangingAids                    = false;
    } m_GfxDebugLayer;
};
#define g_CommandLineOptions CommandLineOptions::GetInstance()
