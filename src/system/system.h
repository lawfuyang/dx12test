#pragma once

class System
{
    DeclareSingletonFunctions(System);

public:
    void Initialize();
    void Shutdown();
    void ProcessWindowsMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Loop();

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

    ::HWND m_EngineWindowHandle = nullptr;

    bool m_Exit = false;

    double m_RealFPS = 0.0;
    double m_CappedFPS = 0.0;
    double m_RealFrameTimeMs   = 0.0;
    double m_CappedFrameTimeMs = 0.0;

    tf::Executor m_Executor;

    uint32_t m_SystemFrameNumber = 0;

    friend class FrameRateController;
};
#define g_System        System::GetInstance()
#define g_TasksExecutor g_System.GetTasksExecutor()

class FrameRateController
{
public:
    ~FrameRateController();

private:
    void BusyWaitUntil(uint64_t tick);

    Timer m_Timer;
};

struct CommandLineOptions
{
    DeclareSingletonFunctions(CommandLineOptions);

    void Parse();

    bool     m_EnableGfxDebugLayer             = false;
    bool     m_GfxMemAllocAlwaysCommitedMemory = false;
    bool     m_PIXCapture                      = false;
    bool     m_ProfileInit                     = false;
    bool     m_ProfileShutdown                 = false;
    uint32_t m_WindowWidth                     = 1600;
    uint32_t m_WindowHeight                    = 900;
};
#define g_CommandLineOptions CommandLineOptions::GetInstance()
