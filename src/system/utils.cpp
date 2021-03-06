#include <system/utils.h>

const char* StringFormat(const char* format, ...)
{
    thread_local char buffer[BBE_KB(1)]{};

    va_list marker;
    va_start(marker, format);
    _vsnprintf_s(buffer, BBE_KB(1), BBE_KB(1), format, marker);
    va_end(marker);

    return buffer;
}

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

const char* GetTimeStamp()
{
    thread_local char dateStr[32]{};
    ::time_t t;
    ::time(&t);
    ::strftime(dateStr, sizeof(dateStr), "%Y_%m_%d_%H_%M_%S", localtime(&t));

    return dateStr;
}

const char* GetLastErrorAsString()
{
    // Get the error message, if any.
    const DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return ""; // No error message has been recorded

    thread_local char buffer[BBE_KB(1)]{};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, sizeof(buffer), NULL);

    return buffer;
}

const char* GetApplicationDirectory()
{
    static StaticString<BBE_KB(1)> s_AppDir = []()
    {
        char fileName[BBE_KB(1)]{};
        ::GetModuleFileNameA(NULL, fileName, sizeof(fileName));
        return GetDirectoryFromPath(fileName).c_str();
    }();

    return s_AppDir.c_str();
}

const char* GetTempDirectory()
{
    static StaticString<BBE_KB(1)> s_TmpDir = StringFormat("%s..\\tmp\\", GetApplicationDirectory());
    return s_TmpDir.c_str();
}

const char* GetAssetsDirectory()
{
    static StaticString<BBE_KB(1)> s_AssetsDir = StringFormat("%s\\assets\\", GetApplicationDirectory());
    return s_AssetsDir.c_str();
}

const std::string GetDirectoryFromPath(const char* fullPath)
{
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

const std::string GetFileExtensionFromPath(const std::string& fullPath)
{
    const std::size_t found = fullPath.find_last_of('.');
    return found == std::string::npos ? "" : fullPath.substr(found + 1);
}

CFileWrapper::CFileWrapper(const char* fileName, bool isReadMode)
{
    m_File = fopen(fileName, isReadMode ? "r" : "w");
}

CFileWrapper::~CFileWrapper()
{
    if (m_File)
    {
        fclose(m_File);
        m_File = nullptr;
    }
}

void ReadDataFromFile(const char* filename, std::vector<std::byte>& data)
{
    bbeProfileFunction();

    assert(data.empty());

    using namespace Microsoft::WRL;

    CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
    extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
    extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    extendedParams.lpSecurityAttributes = nullptr;
    extendedParams.hTemplateFile = nullptr;

    StaticWString<FILENAME_MAX> fileNameW = StringUtils::Utf8ToWide(filename);

    Wrappers::FileHandle file(CreateFile2(fileNameW.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
    if (file.Get() == INVALID_HANDLE_VALUE)
        return;

    FILE_STANDARD_INFO fileInfo = {};
    if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
    {
        assert(false);
    }

    assert(fileInfo.EndOfFile.HighPart == 0);

    const uint32_t size = fileInfo.EndOfFile.LowPart;
    data.resize(size);

    if (!ReadFile(file.Get(), data.data(), fileInfo.EndOfFile.LowPart, nullptr, nullptr))
    {
        assert(false);
    }
}

std::size_t GetFileContentsHash(const char* dir)
{
    // read entire file to array of bytes
    std::vector<std::byte> fileBytes;
    ReadDataFromFile(dir, fileBytes);

    // hash entire array of bytes
    return boost::hash_range(fileBytes.begin(), fileBytes.end());
}

namespace StringUtils
{
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;

    const wchar_t* Utf8ToWide(std::string_view strView)
    {
        thread_local StaticWString<BBE_KB(1)> result;
        result = cvt.from_bytes(strView.data());
        return result.c_str();
    }

    const wchar_t* Utf8ToWideBig(std::string_view strView)
    {
        thread_local std::wstring result;
        result = std::wstring{ strView.data(), strView.data() + strView.length() };
        return result.c_str();
    }

    const char* WideToUtf8(std::wstring_view strView)
    {
        thread_local StaticString<BBE_KB(1)> result;
        result = cvt.to_bytes(strView.data());
        return result.c_str();
    }

    const char* WideToUtf8Big(std::wstring_view strView)
    {
        thread_local std::string result;
        result = cvt.to_bytes(strView.data());
        return result.c_str();
    }
}

template<typename T>
static T GetRandomValue(T range)
{
    static std::random_device randomDevice;
    static std::mt19937_64 randomEngine(randomDevice());

    using distribution = typename std::conditional< std::is_floating_point_v<T>, std::uniform_real_distribution<T>, std::uniform_int_distribution<T>>::type;
    static distribution randomValue(0, std::numeric_limits<T>::max());

    T toret = randomValue(randomEngine);
    if (std::is_floating_point_v<T>)
    {
        toret /= std::numeric_limits<T>::max();
        toret *= range;
    }
    else
    {
        T temp = toret / range;
        toret -= (temp * range);
    }
    return toret;
}

float RandomFloat(float range)
{
    return GetRandomValue<float>(range);
}

uint32_t RandomUInt(uint32_t range)
{
    return GetRandomValue<uint32_t>(range);
}

int32_t RandomInt(uint32_t range)
{
    return GetRandomValue<int32_t>(range);
}
