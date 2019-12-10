#include "system/logger.h"

void Logger::Initialize(const char* outputDir)
{
    ::CreateDirectoryA("../bin", NULL);
    fopen_s(&m_File, outputDir, "w");
}

void Logger::Shutdown()
{
    fclose(m_File);
}
