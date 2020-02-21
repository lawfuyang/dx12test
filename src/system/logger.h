#pragma once

#include <extern/spdlog/spdlog/spdlog.h>
#include <extern/spdlog/spdlog/sinks/basic_file_sink.h>

#include "system/utils.h"

class Logger
{
    DeclareSingletonFunctions(Logger);

public:
    void Initialize(const char* path)
    {
        m_Logger = spdlog::basic_logger_mt("file_logger", path, true);
        m_Logger->flush_on(spdlog::level::level_enum::trace);
        spdlog::set_pattern("[%H:%M:%S] [%^%L%$] %v");
    }

    spdlog::logger& GetLoggerInternal() { return *m_Logger; }

private:
    std::shared_ptr<spdlog::logger> m_Logger;
};
#define g_Log Logger::GetInstance().GetLoggerInternal()
