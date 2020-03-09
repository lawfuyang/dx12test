#pragma once

class System
{
    DeclareSingletonFunctions(System);

public:
    void Initialize();
    void Shutdown();
    void ProcessWindowsMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Loop();

    float GetRealFrameTimeMs()       { return m_RealFrameTimeMs; }
    float GetRealFPS()               { return m_FPS; }
    float GetCappedFrameTimeMs()     { return m_CappedFrameTimeMs; }
    float GetCappedFPS()             { return m_CappedFPS; }
    float GetCappedPrevFrameTimeMs() { return m_CappedPrevFrameMs; }

    uint32_t GetSystemFrameNumber() { return m_SystemFrameNumber; }

    tf::Executor& GetTasksExecutor() { return m_Executor; }
    uint32_t GetCurrentThreadID() const;

private:
    void InitializeThreadIDs();
    void RunKeyboardCommands();

    bool m_Exit             = false;
    uint64_t m_LastUpdateMS = 0;

    StopWatch m_Clock;
    float m_RealFrameTimeMs   = 0.0f;
    float m_FPS               = 0.0f;
    float m_CappedFrameTimeMs = 0.0f;
    float m_CappedFPS         = 0.0f;
    float m_CappedPrevFrameMs = 0.0f;

    tf::Executor m_Executor;

    std::unordered_map<uint32_t, uint32_t> m_STDThreadIDToIndexMap;

    uint32_t m_SystemFrameNumber = 0;

    friend class FrameRateController;
};
#define g_System        System::GetInstance()
#define g_TasksExecutor g_System.GetTasksExecutor()

class FrameRateController
{
public:
    FrameRateController();
    ~FrameRateController();

private:
    StopWatch m_StopWatch;

    std::chrono::high_resolution_clock::time_point m_FrameEndTime;
    std::chrono::high_resolution_clock::time_point m_200FPSFrameEndTime;
};

struct CommandLineOptions
{
    DeclareSingletonFunctions(CommandLineOptions);

    void ParseCmdLine(char*);

    bool     m_PIXCapture      = false;
    bool     m_ProfileInit     = false;
    bool     m_ProfileShutdown = false;
    uint32_t m_WindowWidth     = 1600;  // Add proper cmd line parsing for this
    uint32_t m_WindowHeight    = 900;   // Add proper cmd line parsing for this
};
#define g_CommandLineOptions CommandLineOptions::GetInstance()
