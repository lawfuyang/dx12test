//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "system/memorymappedfile.h"

MemoryMappedFile::MemoryMappedFile() :
    m_mapFile(INVALID_HANDLE_VALUE),
    m_file(INVALID_HANDLE_VALUE),
    m_mapAddress(nullptr),
    m_currentFileSize(0)
{
}

MemoryMappedFile::~MemoryMappedFile()
{
}

void MemoryMappedFile::Init(const std::wstring& filename, UINT fileSize)
{
    m_filename = filename;
    WIN32_FIND_DATA findFileData;
    HANDLE handle = FindFirstFileExW(filename.c_str(), FindExInfoBasic, &findFileData, FindExSearchNameMatch, nullptr, 0);
    bool found = handle != INVALID_HANDLE_VALUE;

    if (found)
    {
        FindClose(handle);
    }

    const std::string fileNameStr = utf8_encode(filename);
    if (found)
    {
        FILE* file = std::fopen(fileNameStr.c_str(), "r");
        assert(file);

        std::byte first4Bytes[4] = {};
        std::fscanf(file, "%c%c%c%c", &first4Bytes[0], &first4Bytes[1], &first4Bytes[2], &first4Bytes[3]);
        std::fclose(file);

        if (*reinterpret_cast<uint32_t*>(first4Bytes) == 0)
        {
            g_Log.error("m_file is an invalid cache file.");
            DeleteFileW(m_filename.c_str());
            found = false;
        }
    }

    m_file = CreateFile2(
        filename.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        (found) ? OPEN_EXISTING : CREATE_NEW,
        nullptr);

    if (m_file == INVALID_HANDLE_VALUE)
    {
        g_Log.error("m_file is invalid. Error {}.", GetLastError());
        g_Log.error("Target file is {}", fileNameStr);
        return;
    }

    LARGE_INTEGER realFileSize = {};
    BOOL flag = GetFileSizeEx(m_file, &realFileSize);
    if (!flag)
    {
        g_Log.error("\nError {} occurred in GetFileSizeEx!", GetLastError());
        assert(false);
        return;
    }

    assert(realFileSize.HighPart == 0);
    m_currentFileSize = realFileSize.LowPart;
    if (m_currentFileSize == 0)
    {
        // File mapping files with a size of 0 produces an error.
        m_currentFileSize = DefaultFileSize;
    }
    else if(fileSize > m_currentFileSize)
    {
        // Grow to the specified size.
        m_currentFileSize = fileSize;
    }

    m_mapFile = CreateFileMapping(m_file, nullptr, PAGE_READWRITE, 0, m_currentFileSize, nullptr);

    if (m_mapFile == nullptr)
    {
        g_Log.error("m_mapFile is NULL: last error: %d\n", GetLastError());
        assert(false);
        return;
    }

    m_mapAddress = MapViewOfFile(m_mapFile, FILE_MAP_ALL_ACCESS, 0, 0, m_currentFileSize);

    if (m_mapAddress == nullptr)
    {
        g_Log.error("m_mapAddress is NULL: last error: %d\n", GetLastError());
        assert(false);
        return;
    }
}

void MemoryMappedFile::Destroy(bool deleteFile)
{
    if (m_mapAddress)
    {
        BOOL flag = UnmapViewOfFile(m_mapAddress);
        if (!flag)
        {
            g_Log.error("\nError %ld occurred unmapping the view!", GetLastError());
            assert(false);
        }

        m_mapAddress = nullptr;

        flag = CloseHandle(m_mapFile);    // Close the file mapping object.
        if (!flag)
        {
            g_Log.error("\nError %ld occurred closing the mapping object!", GetLastError());
            assert(false);
        }

        flag = CloseHandle(m_file);        // Close the file itself.
        if (!flag)
        {
            g_Log.error("\nError %ld occurred closing the file!", GetLastError());
            assert(false);
        }
    }

    if (deleteFile)
    {
        DeleteFileW(m_filename.c_str());
    }
}

void MemoryMappedFile::GrowMapping(UINT size)
{
    // Add space for the extra size at the beginning of the file.
    size += sizeof(UINT);

    // Check the size.
    if (size <= m_currentFileSize)
    {
        // Don't shrink.
        return;
    }

    // Flush.
    BOOL flag = FlushViewOfFile(m_mapAddress, 0);
    if (!flag)
    {
        g_Log.error("\nError %ld occurred flushing the mapping object!", GetLastError());
        assert(false);
    }

    // Close the current mapping.
    Destroy(false);

    // Update the size and create a new mapping.
    m_currentFileSize = size;
    Init(m_filename, m_currentFileSize);
}
