#pragma once

#include "spdlog/spdlog.h"

#include "system/utils.h"

class Logger
{
    DeclareSingletonFunctions(Logger);

public:
    void Initialize(const char* path)
    {
        m_Logger = spdlog::basic_logger_mt("file_logger", path, true);
        m_Logger->flush_on(spdlog::level::level_enum::warn);
        spdlog::set_pattern("[%H:%M:%S] [%^%L%$] %v");
    }

    spdlog::logger& GetLoggerInternal() { return *m_Logger; }

private:
    std::shared_ptr<spdlog::logger> m_Logger;
};

#define g_Log Logger::GetInstance().GetLoggerInternal()
