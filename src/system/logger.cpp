#include "system/logger.h"

void Logger::Initialize()
{
    ::CreateDirectoryA("../bin", NULL);
    fopen_s(&m_File, "../bin/output.txt", "w");
}

void Logger::Shutdown()
{
    fclose(m_File);
}
