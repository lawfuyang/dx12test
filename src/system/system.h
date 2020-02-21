#pragma once

class System
{
    DeclareSingletonFunctions(System);

public:
    static inline const std::string APP_NAME = "DX12 Test";
    static const uint32_t APP_WINDOW_WIDTH   = 1600;
    static const uint32_t APP_WINDOW_HEIGHT  = 900;
    static const uint32_t FPS_LIMIT          = 60;

    void Initialize();
    void Shutdown();
    void ProcessWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Loop();

    static float GetRealFrameTimeMs()       { return ms_RealFrameTimeMs; }
    static float GetRealFPS()               { return ms_FPS; }
    static float GetCappedFrameTimeMs()     { return ms_CappedFrameTimeMs; }
    static float GetCappedFPS()             { return ms_CappedFPS; }
    static float GetCappedPrevFrameTimeMs() { return ms_CappedPrevFrameMs; }

    static uint32_t GetSystemFrameNumber() { return ms_SystemFrameNumber; }

    tf::Executor& GetTasksExecutor() { return m_Executor; }
    uint32_t GetCurrentThreadID() const;

private:
    void InitializeThreadIDs();
    void RunKeyboardCommands();

    bool m_Exit             = false;
    uint64_t m_LastUpdateMS = 0;

    StopWatch m_Clock;
    inline static float ms_RealFrameTimeMs   = 0.0f;
    inline static float ms_FPS               = 0.0f;
    inline static float ms_CappedFrameTimeMs = 0.0f;
    inline static float ms_CappedFPS         = 0.0f;
    inline static float ms_CappedPrevFrameMs = 0.0f;

    tf::Executor m_Executor;

    std::unordered_map<uint32_t, uint32_t> m_STDThreadIDToIndexMap;

    inline static uint32_t ms_SystemFrameNumber = 0;

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

    bool m_PIXCapture = false;
    bool m_ProfileInit = false;
    bool m_ProfileShutdown = false;
};
#define g_CommandLineOptions CommandLineOptions::GetInstance()
