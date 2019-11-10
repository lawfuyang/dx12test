#pragma once

#include "system/utils.h"

#define bbeInfo(str, ...) bbeLogInternal(Info, str, __VA_ARGS__)
#define bbeDebug(str, ...) bbeLogInternal(Debug, str, __VA_ARGS__)
#define bbeLogInternal(Category, str, ...) Logger::GetInstance().Log<Logger::LogCategory::Category>(str, __FUNCTION__, __VA_ARGS__);

#define bbeError(condition, str, ...)   if (!(condition)) bbeLogInternal(Error, str, __VA_ARGS__)
#define bbeWarning(condition, str, ...) if (!(condition)) bbeLogInternal(Warning, str, __VA_ARGS__)
#define bbeAssert(condition, str, ...)                      \
    if (!(condition))                                       \
    {                                                       \
        ::OutputDebugStringA("ASSERT FAILED! Condition: "); \
        ::OutputDebugStringA(bbeTOSTRING(condition));       \
        ::OutputDebugStringA("\n");                         \
        ::OutputDebugStringA(bbeFILEandLINE);               \
        ::OutputDebugStringA("\n");                         \
        bbeLogInternal(Assert, str,__VA_ARGS__);            \
        BreakIntoDebugger();                                \
    }
#define bbeVerify(code, str, ...) bbeAssert((code), str, __VA_ARGS__)

class Logger
{
    DeclareSingletonFunctions(Logger);

public:
    enum LogCategory
    {
        Info,
        Debug,
        Error,
        Warning,
        Assert
    };

    template <LogCategory Category, typename... Args>
    void Log(const char* format, const char* functionSrc, Args&&... args);

    template <LogCategory c>
    static constexpr const char* GetCategoryStr()
    {
        switch (c)
        {
        case Info: return "Info"; break;
        case Debug: return "Debug"; break;
        case Warning: return "Warning"; break;
        case Error: return "Error"; break;
        case Assert: return "Assert"; break;
        default: return "Unknown Log Category";
        }
    }

private:
    FILE* m_File = nullptr;
};

template <Logger::LogCategory Category, typename... Args>
inline void Logger::Log(const char* format, const char* functionSrc, Args&& ... args)
{
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const char* timeNow = std::ctime(&now) + 11; // +11 to remove Day & Month

    const std::string formattedStr = StringFormat(format, std::forward<Args>(args)...);

    // %.8s to remove \n from timeNow
    const std::string prefix = StringFormat("[%.8s] %s[%s]: ", timeNow, GetCategoryStr<Category>(), functionSrc);
    const std::string finalStr = prefix + formattedStr + "\n";

    ::OutputDebugStringA(finalStr.c_str());
    fprintf(m_File, finalStr.c_str());
    fflush(m_File);
}
