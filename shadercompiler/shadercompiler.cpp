#include <vector>
#include <cassert>

#include "system/utils.h"

struct ShaderCompileJob;

std::string                   g_AppDir;
std::string                   g_ShadersDir;
std::string                   g_DXCDir;
std::vector<std::string>      g_AllShaderFiles;
std::vector<ShaderCompileJob> g_AllShaderCompilerJobs;

struct ShaderCompileJob
{
    ShaderCompileJob()
    {
        m_StartupInfo.cb = sizeof(::STARTUPINFO);
    }

    void StartJob()
    {
        std::string commandLine = m_ShaderFilePath;

        if (::CreateProcess(g_DXCDir.c_str(),  // No module name (use command line).
            (LPSTR)commandLine.c_str(),        // Command line.
            nullptr,                           // Process handle not inheritable.
            nullptr,                           // Thread handle not inheritable.
            FALSE,                             // Set handle inheritance to FALSE.
            0,                                 // No creation flags.
            nullptr,                           // Use parent's environment block.
            nullptr,                           // Use parent's starting directory.
            &m_StartupInfo,                    // Pointer to STARTUPINFO structure.
            &m_ProcessInfo))                   // Pointer to PROCESS_INFORMATION structure.
        {
            ::WaitForSingleObject(m_ProcessInfo.hProcess, INFINITE);
            FinishJob();
        }
        else
        {
            assert(false);
        }
    }

    void FinishJob()
    {
        auto SafeCloseHandle = [](::HANDLE& hdl) {if (hdl)::CloseHandle(hdl); hdl = nullptr; };

        SafeCloseHandle(m_StartupInfo.hStdInput);
        SafeCloseHandle(m_StartupInfo.hStdOutput);
        SafeCloseHandle(m_StartupInfo.hStdError);
        SafeCloseHandle(m_ProcessInfo.hProcess);
        SafeCloseHandle(m_ProcessInfo.hThread);
    }

    std::string           m_ShaderFilePath;
    std::string           m_EntryPoint;
    std::string           m_ShaderName;
    ::STARTUPINFO         m_StartupInfo = {};
    ::PROCESS_INFORMATION m_ProcessInfo = {};
};

int main()
{
    g_AppDir     = GetApplicationDirectory();
    g_ShadersDir = g_AppDir + "..\\src\\graphic\\shaders";
    g_DXCDir     = g_AppDir + "..\\tools\\dxc\\dxc.exe";

    GetFilesInDirectory(g_AllShaderFiles, g_ShadersDir);
    
    for (const std::string& fullPath : g_AllShaderFiles)
    {
        printf("Found shader file: %s\n", fullPath.c_str());

        ShaderCompileJob newJob;
        newJob.m_ShaderFilePath = fullPath;

        g_AllShaderCompilerJobs.push_back(std::move(newJob));
    }
    printf("\n");

    for (ShaderCompileJob& shaderJob : g_AllShaderCompilerJobs)
    {
        shaderJob.StartJob();
    }

    system("pause");
    return 0;
}
