#include "system/utils.h"

#include <assert.h>

void BreakIntoDebugger()
{
    if (!::IsDebuggerPresent())
    {
        if (::MessageBox(nullptr, "Attach your debugger *before* pressing OK to debug\r\nor press Cancel to skip the debug request.", "Assert!", MB_OKCANCEL | MB_ICONEXCLAMATION | MB_SETFOREGROUND) != IDOK)
        {
            std::abort();
        }
    }
    __debugbreak();
}

const std::string GetTimeStamp()
{
    char dateStr[32] = { 0 };
    ::time_t t;
    ::time(&t);
    ::strftime(dateStr, sizeof(dateStr), "%Y_%m_%d_%H_%M_%S", localtime(&t));

    return dateStr;
}

const std::string utf8_encode(const std::wstring& wstr)
{
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

const std::wstring utf8_decode(const std::string& str)
{
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

const std::string GetLastErrorAsString()
{
    // Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return ""; //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    return std::string{ messageBuffer };
}

const std::string GetApplicationDirectory()
{
    static std::once_flag s_OnceFlag;
    static std::string s_AppDir;

    std::call_once(s_OnceFlag, [&]()
        {
            CHAR fileName[1024] = {};
            ::GetModuleFileNameA(NULL, fileName, sizeof(fileName));
            s_AppDir = GetDirectoryFromPath(fileName);
        });

    return s_AppDir;
}

const std::string GetTempDirectory()
{
    static std::string s_TmpDir = GetApplicationDirectory() + "..\\tmp\\";
    return s_TmpDir;
}

const std::string GetDirectoryFromPath(const std::string& fullPath)
{
    if (fullPath.empty())
    {
        return fullPath;
    }

    std::string ret, empty;
    SplitPath(fullPath, ret, empty);
    return ret;
}

void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName)
{
    const std::size_t found = fullPath.find_last_of("/\\");
    dir = fullPath.substr(0, found + 1);
    fileName = fullPath.substr(found + 1);
}

void GetFilesInDirectory(std::vector<std::string>& out, const std::string& directory)
{
    ::HANDLE dir;
    ::WIN32_FIND_DATA file_data;

    if ((dir = ::FindFirstFile((directory + "\\*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
        return; /* No files found */

    do
    {
        const std::string file_name = file_data.cFileName;
        const std::string full_file_name = directory + "\\" + file_name;
        const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (file_name[0] == '.')
            continue;

        //if (is_directory)
        //  continue;

        out.push_back(full_file_name);
    } while (FindNextFile(dir, &file_data));

    FindClose(dir);
}

const std::string GetFileNameFromPath(const std::string& fullPath)
{
    if (fullPath.empty())
    {
        return fullPath;
    }

    std::string ret, empty;
    SplitPath(fullPath, empty, ret);
    return ret;
}

CFileWrapper::CFileWrapper(const std::string& fileName, bool isReadMode)
{
    m_File = fopen(fileName.c_str(), isReadMode ? "r" : "w");
}

CFileWrapper::~CFileWrapper()
{
    if (m_File)
    {
        fclose(m_File);
        m_File = nullptr;
    }
}

MultithreadDetector::MultithreadDetector()
{
    const std::thread::id newID = std::this_thread::get_id();

    if (ms_CurrentID != std::thread::id{} && newID != ms_CurrentID)
    {
        assert(false); // Multi-thread detected!
    }

    ms_CurrentID = newID;
}

MultithreadDetector::~MultithreadDetector()
{
    ms_CurrentID = std::thread::id{};
}
