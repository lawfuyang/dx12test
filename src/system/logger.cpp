#include "system/precomp.h"
#include "system/logger.h"

Logger::Logger()
{
    ::CreateDirectoryA("../bin", NULL);
    fopen_s(&m_File, "../bin/output.txt", "w");
}

Logger::~Logger()
{
    fclose(m_File);
}
