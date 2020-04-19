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

    double GetRealFrameTimeMs()       { return m_RealFrameTimeMs; }
    double GetRealFPS()               { return m_RealFPS; }
    double GetCappedFrameTimeMs()     { return m_CappedFrameTimeMs; }
    double GetCappedFPS()             { return m_CappedFPS; }

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

    double m_RealFPS = 0.0;
    double m_CappedFPS = 0.0;
    double m_RealFrameTimeMs   = 0.0;
    double m_CappedFrameTimeMs = 0.0;

    tf::Executor m_Executor;
    std::thread m_BGAsyncThread;

    Timer m_FrameTimer;
    uint32_t m_SystemFrameNumber = 0;
};
#define g_System                    System::GetInstance()
#define g_TasksExecutor             g_System.GetTasksExecutor()
#define ADD_TF_TASK(taskFlow, task) taskFlow.emplace([&]() { task; }).name(bbeTOSTRING(task))
#define ADD_SF_TASK(taskFlow, task) taskFlow.emplace([&](tf::Subflow& sf) { task; }).name(bbeTOSTRING(task))

struct CommandLineOptions
{
    DeclareSingletonFunctions(CommandLineOptions);

    void Parse();

    bool     m_RunUnitTests                    = false;
    bool     m_EnableGfxDebugLayer             = false;
    bool     m_GfxMemAllocAlwaysCommitedMemory = false;
    bool     m_PIXCapture                      = false;
    bool     m_ProfileInit                     = false;
    bool     m_ProfileShutdown                 = false;
    uint32_t m_WindowWidth                     = 1600;
    uint32_t m_WindowHeight                    = 900;
};
#define g_CommandLineOptions CommandLineOptions::GetInstance()
